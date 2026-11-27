#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcIsotropicSpecularAAPatcher.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "Core/ShaderIdentityMatcher.h"

#include <algorithm>
#include <cstddef>
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
	switch (code)
	{
	case BadMagic:
		return "BadMagic";
	case HeaderTruncated:
		return "HeaderTruncated";
	case DeclaredSizeMismatch:
		return "DeclaredSizeMismatch";
	case ChunkCountOverflow:
		return "ChunkCountOverflow";
	case ChunkTableOutOfRange:
		return "ChunkTableOutOfRange";
	case ChunkHeaderOutOfRange:
		return "ChunkHeaderOutOfRange";
	case ChunkOutOfRange:
		return "ChunkOutOfRange";
	case ChunksOverlap:
		return "ChunksOverlap";
	case MissingShaderChunk:
		return "MissingShaderChunk";
	case DuplicateShaderChunk:
		return "DuplicateShaderChunk";
	case ShaderLengthMismatch:
		return "ShaderLengthMismatch";
	case UnsupportedCustomData:
		return "UnsupportedCustomData";
	case ZeroInstructionLength:
		return "ZeroInstructionLength";
	case InstructionOutOfRange:
		return "InstructionOutOfRange";
	case InternalError:
		return "InternalError";
	case TargetSizeMismatch:
		return "TargetSizeMismatch";
	case HashMismatch:
		return "HashMismatch";
	case TargetRegionMissing:
		return "TargetRegionMissing";
	case AlreadyPatched:
		return "AlreadyPatched";
	}
	return "Unknown";
}

auto Fail(ExitCode code, const std::string &message) -> int
{
	std::cerr << "IsotropicShaderPatcher: error: " << message << '\n';
	return static_cast<int>(code);
}

auto ReadAllBytes(const std::filesystem::path &path) -> std::vector<std::byte>
{
	std::ifstream stream(path, std::ios::binary | std::ios::ate);
	if (!stream.is_open())
	{
		throw std::runtime_error("open failed");
	}
	const auto size = static_cast<std::streamoff>(stream.tellg());
	if (size < 0)
	{
		throw std::runtime_error("size query failed");
	}
	std::vector<std::byte> bytes(static_cast<std::size_t>(size));
	stream.seekg(0, std::ios::beg);
	if (!stream || (size > 0 && !stream.read(reinterpret_cast<char *>(bytes.data()), size)))
	{
		throw std::runtime_error("read failed");
	}
	return bytes;
}
} // namespace

auto main(int argc, char *argv[]) -> int
{
	try
	{
		if (argc != 3 && argc != 5)
		{
			std::cerr << "usage: IsotropicShaderPatcher <original.dxbc> <output.dxbc> [--variant-of <OG hash>]\n";
			return static_cast<int>(ExitCode::Usage);
		}
		std::string_view variantOf;
		if (argc == 5)
		{
			if (std::string_view(argv[3]) != "--variant-of")
			{
				return static_cast<int>(ExitCode::Usage);
			}
			variantOf = argv[4];
		}
		const std::filesystem::path inputPath = argv[1];
		const std::filesystem::path outputPath = argv[2];
		if (std::filesystem::weakly_canonical(inputPath) == std::filesystem::weakly_canonical(outputPath))
		{
			return Fail(ExitCode::WriteFailure, "output path must differ from the read-only input path");
		}

		const auto original = ReadAllBytes(inputPath);
		auto identity = NaNBuster::IsotropicSpecularTargetIdentity;
		if (!variantOf.empty())
		{
			if (variantOf != NaNBuster::IsotropicSpecularReferenceStem)
			{
				return Fail(ExitCode::IdentityFailure,
							"unknown --variant-of identity '" + std::string(variantOf) + "'");
			}
			identity.BytecodeSize = static_cast<std::uint32_t>(original.size());
			std::copy_n(original.begin() + 4, identity.EmbeddedHash.size(), identity.EmbeddedHash.begin());
		}
		else if (!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{original.data(), original.size()},
															identity))
		{
			return Fail(ExitCode::IdentityFailure, "input is not the exact original isotropic specular target");
		}

		const NaNBuster::DxbcInspector inspector;
		auto inspected = inspector.Inspect(NaNBuster::ByteView{original.data(), original.size()});
		if (!inspected)
		{
			return Fail(ExitCode::MalformedInput,
						"container inspection failed: " + std::string(ErrorName(inspected.error().Code)) +
							" at offset " + std::to_string(inspected.error().Offset));
		}

		NaNBuster::IsotropicSpecularAAVariantLayout layout;
		if (!variantOf.empty())
		{
			layout.CullTokenOffset = 325;
			layout.DecodeTokenOffsets = {380, 395, 402, 418, 423, 430};
			layout.ExponentMadTokenOffset = 1002;
			layout.ExponentTokenOffset = 1011;
			layout.ExponentInsertionTokenOffset = 1016;
			layout.ExponentRegister = 0;
			layout.ExponentComponent = 1;
		}
		auto patched = variantOf.empty()
						   ? NaNBuster::DxbcIsotropicSpecularAAPatcher{}.Patch(*inspected, identity)
						   : NaNBuster::DxbcIsotropicSpecularAAPatcher{}.PatchVariant(*inspected, identity, layout);
		if (!patched)
		{
			return Fail(ExitCode::PatchFailure,
						"isotropic AA patch failed: " + std::string(ErrorName(patched.error().Code)) + " at offset " +
							std::to_string(patched.error().Offset));
		}
		auto finalized = NaNBuster::DxbcHashWriter{}.Write(patched->Bytes);
		if (!finalized)
		{
			return Fail(ExitCode::HashFailure, "hash finalization failed");
		}
		if (patched->Bytes.size() != original.size() + 544)
		{
			return Fail(ExitCode::OutputValidationFailure, "corrected size does not match original size + 544 bytes");
		}
		if (std::memcmp(original.data() + 4, patched->Bytes.data() + 4, 16) == 0)
		{
			return Fail(ExitCode::OutputValidationFailure, "corrected embedded hash was not regenerated");
		}
		auto reinspected = inspector.Inspect(NaNBuster::ByteView{patched->Bytes.data(), patched->Bytes.size()});
		if (!reinspected)
		{
			return Fail(ExitCode::OutputValidationFailure, "finalized container failed re-inspection");
		}

		{
			std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
			if (!stream.is_open())
			{
				return Fail(ExitCode::WriteFailure, "failed to open output '" + outputPath.string() + "'");
			}
			stream.write(reinterpret_cast<const char *>(patched->Bytes.data()),
						 static_cast<std::streamsize>(patched->Bytes.size()));
			stream.flush();
			if (!stream)
			{
				return Fail(ExitCode::WriteFailure, "failed to write output '" + outputPath.string() + "'");
			}
		}
		const auto written = ReadAllBytes(outputPath);
		if (written != patched->Bytes)
		{
			return Fail(ExitCode::WriteFailure, "output read-back mismatch for '" + outputPath.string() + "'");
		}

		std::cout << "IsotropicShaderPatcher: wrote " << written.size() << " corrected bytes to '"
				  << outputPath.string() << "'\n";
		return static_cast<int>(ExitCode::Success);
	}
	catch (const std::exception &error)
	{
		return Fail(ExitCode::WriteFailure, std::string("unexpected failure: ") + error.what());
	}
	catch (...)
	{
		return Fail(ExitCode::WriteFailure, "unexpected unknown failure");
	}
}
