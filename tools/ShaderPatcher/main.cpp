// NaNBuster offline corrected-DXBC generator (Chunk C6).
// Reads the read-only original reference, validates its identity, runs the
// pure-Core inspect/patch/hash-finalize pipeline, validates the corrected
// output, and writes it deterministically. Fails clearly (non-zero exit,
// stderr message) on wrong identity, malformed data, patch/hash failure, or
// output I/O failure. Never modifies its input; only the explicit output
// path is written.

#include "Core/DxbcClampPatcher.h"
#include "Core/DxbcContainer.h"
#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/ShaderIdentity.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// Independently proven corrected-container values (G3_CLAMP_EVIDENCE.md,
	// G3_HASH_EVIDENCE.md): 1612 + 40 inserted bytes, regenerated embedded
	// hash c6e8de1dd30168fb0b4bd6eb9ba109fa stored at container offset 4.
	constexpr std::size_t ExpectedCorrectedSize = 1652;
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;
	constexpr std::array<std::byte, 16> ExpectedCorrectedEmbeddedHash = {
		std::byte{ 0xc6 }, std::byte{ 0xe8 }, std::byte{ 0xde }, std::byte{ 0x1d },
		std::byte{ 0xd3 }, std::byte{ 0x01 }, std::byte{ 0x68 }, std::byte{ 0xfb },
		std::byte{ 0x0b }, std::byte{ 0x4b }, std::byte{ 0xd6 }, std::byte{ 0xeb },
		std::byte{ 0x9b }, std::byte{ 0xa1 }, std::byte{ 0x09 }, std::byte{ 0xfa },
	};

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
		std::cerr << "ShaderPatcher: error: " << message << '\n';
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
			std::cerr << "usage: ShaderPatcher <original.dxbc> <output.dxbc>\n";
			return static_cast<int>(ExitCode::Usage);
		}

		const std::filesystem::path inputPath = argv[1];
		const std::filesystem::path outputPath = argv[2];

		if (std::filesystem::weakly_canonical(inputPath) == std::filesystem::weakly_canonical(outputPath)) {
			return Fail(ExitCode::WriteFailure, "output path must differ from the read-only input path");
		}

		const std::vector<std::byte> original = ReadAllBytes(inputPath);

		// Original identity gate: exact size and embedded hash (offset 4).
		if (original.size() != NaNBuster::OriginalTargetIdentity.BytecodeSize) {
			return Fail(
				ExitCode::IdentityFailure,
				"input size " + std::to_string(original.size()) + " does not match target size " +
					std::to_string(NaNBuster::OriginalTargetIdentity.BytecodeSize) + " (" +
					std::string(NaNBuster::OriginalTargetIdentity.ShaderUid) + ")");
		}
		if (std::memcmp(
				original.data() + DxbcEmbeddedHashOffset,
				NaNBuster::OriginalTargetIdentity.EmbeddedHash.data(),
				NaNBuster::OriginalTargetIdentity.EmbeddedHash.size()) != 0) {
			return Fail(ExitCode::IdentityFailure, "embedded hash at offset 4 does not match the approved target identity");
		}

		const NaNBuster::DxbcInspector inspector;
		auto inspected = inspector.Inspect(NaNBuster::ByteView{ original.data(), original.size() });
		if (!inspected) {
			const NaNBuster::DxbcError& error = inspected.error();
			return Fail(
				ExitCode::MalformedInput,
				"container inspection failed: " + std::string(ErrorName(error.Code)) + " at offset " + std::to_string(error.Offset));
		}

		const NaNBuster::DxbcClampPatcher patcher;
		auto patched = patcher.Patch(*inspected);
		if (!patched) {
			const NaNBuster::DxbcError& error = patched.error();
			return Fail(
				ExitCode::PatchFailure,
				"clamp patch failed: " + std::string(ErrorName(error.Code)) + " at offset " + std::to_string(error.Offset));
		}

		const NaNBuster::DxbcHashWriter hashWriter;
		auto finalized = hashWriter.Write(patched->Bytes);
		if (!finalized) {
			const NaNBuster::DxbcError& error = finalized.error();
			return Fail(
				ExitCode::HashFailure,
				"hash finalization failed: " + std::string(ErrorName(error.Code)) + " at offset " + std::to_string(error.Offset));
		}

		// Output validation before any write: exact size, regenerated embedded
		// hash, and a successful re-inspection of the finalized container.
		if (patched->Bytes.size() != ExpectedCorrectedSize) {
			return Fail(
				ExitCode::OutputValidationFailure,
				"corrected size " + std::to_string(patched->Bytes.size()) + " does not match expected " +
					std::to_string(ExpectedCorrectedSize));
		}
		if (std::memcmp(
				patched->Bytes.data() + DxbcEmbeddedHashOffset,
				ExpectedCorrectedEmbeddedHash.data(),
				ExpectedCorrectedEmbeddedHash.size()) != 0) {
			return Fail(ExitCode::OutputValidationFailure, "corrected embedded hash does not match the proven oracle c6e8de1dd30168fb0b4bd6eb9ba109fa");
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

		// Post-write read-back verification (output I/O failure detection).
		const std::vector<std::byte> written = ReadAllBytes(outputPath);
		if (written.size() != patched->Bytes.size() ||
			std::memcmp(written.data(), patched->Bytes.data(), patched->Bytes.size()) != 0) {
			return Fail(ExitCode::WriteFailure, "output read-back mismatch for '" + outputPath.string() + "'");
		}

		std::cout << "ShaderPatcher: wrote " << written.size() << " corrected bytes to '" << outputPath.string() << "'\n";
		return static_cast<int>(ExitCode::Success);
	} catch (const std::exception& error) {
		return Fail(ExitCode::WriteFailure, std::string("unexpected failure: ") + error.what());
	} catch (...) {
		return Fail(ExitCode::WriteFailure, "unexpected unknown failure");
	}
}
