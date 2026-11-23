#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "Core/LambertShaderIdentities.h"
#include "Core/ShaderIdentityMatcher.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	enum class ExitCode : int
	{
		Success = 0,
		Usage = 2,
		ReadFailure = 3,
		IdentityFailure = 4,
		MalformedInput = 5,
		PatchFailure = 6,
		HashFailure = 7,
		OutputValidationFailure = 8,
		WriteFailure = 9,
	};

	auto ErrorName(NaNBuster::DxbcErrorCode code) -> std::string_view
	{
		using enum NaNBuster::DxbcErrorCode;
		switch (code) {
		case BadMagic: return "BadMagic";
		case HeaderTruncated: return "HeaderTruncated";
		case DeclaredSizeMismatch: return "DeclaredSizeMismatch";
		case ChunkCountOverflow: return "ChunkCountOverflow";
		case ChunkTableOutOfRange: return "ChunkTableOutOfRange";
		case ChunkHeaderOutOfRange: return "ChunkHeaderOutOfRange";
		case ChunkOutOfRange: return "ChunkOutOfRange";
		case ChunksOverlap: return "ChunksOverlap";
		case MissingShaderChunk: return "MissingShaderChunk";
		case DuplicateShaderChunk: return "DuplicateShaderChunk";
		case ShaderLengthMismatch: return "ShaderLengthMismatch";
		case UnsupportedCustomData: return "UnsupportedCustomData";
		case ZeroInstructionLength: return "ZeroInstructionLength";
		case InstructionOutOfRange: return "InstructionOutOfRange";
		case InternalError: return "InternalError";
		case TargetSizeMismatch: return "TargetSizeMismatch";
		case HashMismatch: return "HashMismatch";
		case TargetRegionMissing: return "TargetRegionMissing";
		case AlreadyPatched: return "AlreadyPatched";
		}
		return "Unknown";
	}

	auto Fail(ExitCode code, const std::string& message) -> int
	{
		std::cerr << "LambertShaderPatcher: error: " << message << '\n';
		return static_cast<int>(code);
	}

	auto ReadAllBytes(const std::filesystem::path& path) -> std::vector<std::byte>
	{
		std::ifstream stream(path, std::ios::binary | std::ios::ate);
		if (!stream.is_open()) {
			throw std::runtime_error("open failed");
		}
		const auto size = static_cast<std::streamoff>(stream.tellg());
		if (size < 0) {
			throw std::runtime_error("size query failed");
		}
		std::vector<std::byte> bytes(static_cast<std::size_t>(size));
		stream.seekg(0, std::ios::beg);
		if (!stream || (size > 0 && !stream.read(reinterpret_cast<char*>(bytes.data()), size))) {
			throw std::runtime_error("read failed");
		}
		return bytes;
	}
}

auto main(int argc, char* argv[]) -> int
{
	try {
		if (argc != 3) {
			std::cerr << "usage: LambertShaderPatcher <original.dxbc> <output.dxbc>\n";
			return static_cast<int>(ExitCode::Usage);
		}

		const std::filesystem::path inputPath = argv[1];
		const std::filesystem::path outputPath = argv[2];
		if (std::filesystem::weakly_canonical(inputPath) == std::filesystem::weakly_canonical(outputPath)) {
			return Fail(ExitCode::WriteFailure, "output path must differ from the read-only input path");
		}

		const auto original = ReadAllBytes(inputPath);
		const auto identity = std::find_if(
			NaNBuster::LambertShaderIdentities.begin(),
			NaNBuster::LambertShaderIdentities.end(),
			[&original](const auto& candidate) {
				return NaNBuster::ShaderIdentityMatcher::Matches(
					NaNBuster::ByteView{ original.data(), original.size() }, candidate.Identity);
			});
		if (identity == NaNBuster::LambertShaderIdentities.end()) {
			return Fail(ExitCode::IdentityFailure, "input is not one of the eleven approved original Lambert decoders");
		}

		const NaNBuster::DxbcInspector inspector;
		auto inspected = inspector.Inspect(NaNBuster::ByteView{ original.data(), original.size() });
		if (!inspected) {
			return Fail(
				ExitCode::MalformedInput,
				"container inspection failed: " + std::string(ErrorName(inspected.error().Code)) +
					" at offset " + std::to_string(inspected.error().Offset));
		}

		const NaNBuster::DxbcLambertGuardPatcher patcher;
		auto patched = patcher.Patch(*inspected, *identity);
		if (!patched) {
			return Fail(
				ExitCode::PatchFailure,
				"guard patch failed: " + std::string(ErrorName(patched.error().Code)) +
					" at offset " + std::to_string(patched.error().Offset));
		}

		const NaNBuster::DxbcHashWriter hashWriter;
		auto finalized = hashWriter.Write(patched->Bytes);
		if (!finalized) {
			return Fail(
				ExitCode::HashFailure,
				"hash finalization failed: " + std::string(ErrorName(finalized.error().Code)) +
					" at offset " + std::to_string(finalized.error().Offset));
		}
		if (patched->Bytes.size() != original.size() + 168) {
			return Fail(ExitCode::OutputValidationFailure, "corrected size is not original size + 168");
		}
		auto reinspected = inspector.Inspect(NaNBuster::ByteView{ patched->Bytes.data(), patched->Bytes.size() });
		if (!reinspected) {
			return Fail(ExitCode::OutputValidationFailure, "finalized container failed re-inspection");
		}

		{
			std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
			if (!stream.is_open()) {
				return Fail(ExitCode::WriteFailure, "failed to open output '" + outputPath.string() + "'");
			}
			stream.write(reinterpret_cast<const char*>(patched->Bytes.data()), static_cast<std::streamsize>(patched->Bytes.size()));
			stream.flush();
			if (!stream) {
				return Fail(ExitCode::WriteFailure, "failed to write output '" + outputPath.string() + "'");
			}
		}

		const auto written = ReadAllBytes(outputPath);
		if (written != patched->Bytes) {
			return Fail(ExitCode::WriteFailure, "output read-back mismatch for '" + outputPath.string() + "'");
		}

		std::cout << "LambertShaderPatcher: " << identity->Identity.ShaderUid << " wrote "
				  << written.size() << " corrected bytes to '" << outputPath.string() << "'\n";
		return static_cast<int>(ExitCode::Success);
	} catch (const std::exception& error) {
		return Fail(ExitCode::WriteFailure, std::string("unexpected failure: ") + error.what());
	} catch (...) {
		return Fail(ExitCode::WriteFailure, "unexpected unknown failure");
	}
}
