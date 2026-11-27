#include "Core/AEShaderVariantIdentities.h"
#include "Core/DxbcAnisotropicSpecularClampPatcher.h"
#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcIsotropicSpecularAAPatcher.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "TestShaderFixtures.h"

#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/anisotropic-clamped.inc"
#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/combined.inc"
#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/anisotropic-clamped.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/combined.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "AEShaderVariants/4bfc847c-48cbb854-83d96138-488b9bb1/patched.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/anisotropic-clamped.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/combined.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/anisotropic-clamped.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/combined.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/anisotropic-clamped.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/combined.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/anisotropic-clamped.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/combined.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "AEShaderVariants/bcc0320d-dee924ba-4d26b206-84d74f3f/patched.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/anisotropic-clamped.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/combined.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/patched.inc"
#include "AEShaderVariants/d75dfae6-af7b9aa5-1a0374d8-3d186619/patched.inc"
#include "AEShaderVariants/def41f6e-9dbcf63b-4c43cf22-4eea0bb4/patched.inc"
#include "AEShaderVariants/e73d9a26-0c9afe98-f628f975-0e5f8aef/patched.inc"
#include "AEShaderVariants/f4b2da50-c279fcd9-bd60ba4f-6a804632/patched.inc"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <d3d11.h>

#ifndef NANBUSTER_AE_REFERENCE_ROOT
#error "NANBUSTER_AE_REFERENCE_ROOT is required."
#endif
#ifndef NANBUSTER_AE_SHADER_OUTPUT_ROOT
#error "NANBUSTER_AE_SHADER_OUTPUT_ROOT is required."
#endif

namespace
{
using Bytes = std::vector<std::byte>;

enum class PatchMode
{
	Lambert,
	Anisotropic,
	Combined,
};

struct GeneratedPayload
{
	const std::uint8_t *Data;
	std::size_t Size;
};

int g_failures = 0;

void Check(bool condition, const char *expression, const char *file, int line)
{
	if (!condition)
	{
		++g_failures;
		std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

template <std::size_t N> constexpr auto PayloadOf(const std::array<std::uint8_t, N> &value) -> GeneratedPayload
{
	return {value.data(), value.size()};
}

auto GeneratedFor(std::string_view stem, PatchMode mode) -> GeneratedPayload
{
	using namespace NaNBuster::Generated;
	if (stem == "bcc0320d-dee924ba-4d26b206-84d74f3f")
		return PayloadOf(AE_Lambert_bcc0320d);
	if (stem == "def41f6e-9dbcf63b-4c43cf22-4eea0bb4")
		return PayloadOf(AE_Lambert_def41f6e);
	if (stem == "d75dfae6-af7b9aa5-1a0374d8-3d186619")
		return PayloadOf(AE_Lambert_d75dfae6);
	if (stem == "4bfc847c-48cbb854-83d96138-488b9bb1")
		return PayloadOf(AE_Lambert_4bfc847c);
	if (stem == "e73d9a26-0c9afe98-f628f975-0e5f8aef")
		return PayloadOf(AE_Lambert_e73d9a26);
	if (stem == "8904fc0f-cb497158-117bc46d-4e1cf466")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_8904fc0f);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_8904fc0f);
		return PayloadOf(AE_Combined_8904fc0f);
	}
	if (stem == "4dc83306-024b9074-9c24d67a-fcdee952")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_4dc83306);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_4dc83306);
		return PayloadOf(AE_Combined_4dc83306);
	}
	if (stem == "325742ce-45bf17d0-f4879324-3637b6aa")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_325742ce);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_325742ce);
		return PayloadOf(AE_Combined_325742ce);
	}
	if (stem == "600e22f5-e1a79678-b183a8f0-99de971d")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_600e22f5);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_600e22f5);
		return PayloadOf(AE_Combined_600e22f5);
	}
	if (stem == "6b7aa1fd-0175af05-1bf184b7-22dd8846")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_6b7aa1fd);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_6b7aa1fd);
		return PayloadOf(AE_Combined_6b7aa1fd);
	}
	if (stem == "262c37a4-f4966781-271cf8ba-64c06ec1")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_262c37a4);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_262c37a4);
		return PayloadOf(AE_Combined_262c37a4);
	}
	if (stem == "be21279d-733505d8-43f98a86-09502d65")
	{
		if (mode == PatchMode::Lambert)
			return PayloadOf(AE_Lambert_be21279d);
		if (mode == PatchMode::Anisotropic)
			return PayloadOf(AE_Anisotropic_be21279d);
		return PayloadOf(AE_Combined_be21279d);
	}
	throw std::runtime_error("unknown generated AE payload");
}

auto ReadLines(const std::filesystem::path &path) -> std::vector<std::string>
{
	std::ifstream stream(path);
	if (!stream.is_open())
	{
		throw std::runtime_error("failed to open ASM: " + path.string());
	}
	std::vector<std::string> lines;
	for (std::string line; std::getline(stream, line);)
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		const auto first = line.find_first_not_of(" \t");
		if (first == std::string::npos || line.starts_with("Shader hash") || line.starts_with("//"))
		{
			continue;
		}
		const auto last = line.find_last_not_of(" \t");
		lines.push_back(line.substr(first, last - first + 1));
	}
	return lines;
}

auto TempCount(const NaNBuster::DxbcContainerView &container) -> std::uint32_t
{
	for (const auto &instruction : container.Instructions())
	{
		if (instruction.Opcode == 104)
		{
			const auto &stream = container.ShaderStream();
			const std::size_t offset = (instruction.TokenOffset + 1) * 4;
			return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset])) |
				   (static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 1])) << 8) |
				   (static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 2])) << 16) |
				   (static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 3])) << 24);
		}
	}
	return 0;
}

bool HasGuard(const NaNBuster::DxbcContainerView &container)
{
	const auto instructions = container.Instructions();
	for (std::size_t i = 0; i + 5 < instructions.size(); ++i)
	{
		if (instructions[i].Opcode == 49 && instructions[i + 1].Opcode == 68 && instructions[i + 2].Opcode == 56 &&
			instructions[i + 3].Opcode == 55 && instructions[i + 4].Opcode == 56 && instructions[i + 5].Opcode == 15)
		{
			return true;
		}
	}
	return false;
}

void CheckLambertAsmDiff(const std::filesystem::path &originalPath, const std::filesystem::path &correctedPath)
{
	const auto original = ReadLines(originalPath);
	const auto corrected = ReadLines(correctedPath);
	CHECK(corrected.size() == original.size() + 6);
	if (corrected.size() != original.size() + 6)
	{
		return;
	}

	std::size_t mad = original.size();
	for (std::size_t i = 0; i + 1 < original.size(); ++i)
	{
		if (original[i].starts_with("mad ") && original[i].contains("4.000000") && original[i].contains("-2.000000") &&
			original[i + 1].starts_with("dp2 "))
		{
			CHECK(mad == original.size());
			mad = i;
		}
	}
	CHECK(mad != original.size());
	if (mad == original.size())
	{
		return;
	}
	const std::size_t guard = mad + 2;
	const std::array<std::string_view, 6> opcodes{"lt ", "rsq ", "mul ", "movc ", "mul ", "dp2 "};
	for (std::size_t i = 0; i < opcodes.size(); ++i)
	{
		CHECK(corrected[guard + i].starts_with(opcodes[i]));
	}
	CHECK(corrected[guard + 2].contains("l(1.999000)"));
	CHECK(original[guard - 1].starts_with("dp2 "));
	CHECK(corrected[guard - 1] == original[guard - 1]);
	CHECK(corrected[guard + 6] == original[guard]);

	for (std::size_t i = 0; i < corrected.size(); ++i)
	{
		if ((i >= guard && i < guard + 6) || corrected[i].starts_with("dcl_temps"))
		{
			continue;
		}
		const std::size_t expected = i < guard ? i : i - 6;
		if (original[expected].starts_with("dcl_temps"))
		{
			continue;
		}
		CHECK(corrected[i] == original[expected]);
	}
}

void CheckAnisotropicAsmDiff(const std::filesystem::path &originalPath, const std::filesystem::path &correctedPath)
{
	const auto original = ReadLines(originalPath);
	const auto corrected = ReadLines(correctedPath);
	CHECK(corrected.size() == original.size() + 1);
	if (corrected.size() != original.size() + 1)
	{
		return;
	}

	std::size_t anchor = original.size();
	for (std::size_t i = 0; i + 4 < original.size(); ++i)
	{
		if (original[i].starts_with("max ") && original[i + 1].starts_with("log ") &&
			original[i + 2].starts_with("mul ") && original[i + 3].starts_with("exp ") &&
			original[i + 4].starts_with("mul "))
		{
			CHECK(anchor == original.size());
			anchor = i;
		}
	}
	CHECK(anchor != original.size());
	if (anchor == original.size())
	{
		return;
	}
	const auto destinationEnd = original[anchor].find(',');
	CHECK(destinationEnd != std::string::npos);
	const auto destination = original[anchor].substr(4, destinationEnd - 4);
	CHECK(corrected[anchor] == original[anchor]);
	CHECK(corrected[anchor + 1] == "min " + destination + ", " + destination + ", l(1.000000)");
	for (std::size_t i = 0; i < corrected.size(); ++i)
	{
		if (i == anchor + 1)
		{
			continue;
		}
		const std::size_t expected = i <= anchor ? i : i - 1;
		CHECK(corrected[i] == original[expected]);
	}
}

struct ReleaseDeleter
{
	template <typename T> void operator()(T *pointer) const
	{
		if (pointer != nullptr)
		{
			pointer->Release();
		}
	}
};

void CheckD3D(ID3D11Device *device, const NaNBuster::LambertShaderIdentity &identity, const Bytes &corrected)
{
	if (device == nullptr)
	{
		return;
	}
	HRESULT result = E_FAIL;
	if (identity.Stage == NaNBuster::LambertShaderStage::Pixel)
	{
		ID3D11PixelShader *raw = nullptr;
		result = device->CreatePixelShader(corrected.data(), corrected.size(), nullptr, &raw);
		std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> shader(raw);
	}
	else
	{
		ID3D11ComputeShader *raw = nullptr;
		result = device->CreateComputeShader(corrected.data(), corrected.size(), nullptr, &raw);
		std::unique_ptr<ID3D11ComputeShader, ReleaseDeleter> shader(raw);
	}
	if (FAILED(result))
	{
		std::cerr << identity.Identity.ShaderUid << " D3D creation failed: 0x" << std::hex << result << std::dec
				  << '\n';
	}
	CHECK(SUCCEEDED(result));
}

void ValidateTarget(const NaNBuster::LambertShaderIdentity &identity, ID3D11Device *device, bool anisotropic)
{
	const std::filesystem::path referenceRoot = NANBUSTER_AE_REFERENCE_ROOT;
	const std::filesystem::path outputRoot = NANBUSTER_AE_SHADER_OUTPUT_ROOT;
	const auto targetRoot = outputRoot / identity.ReferenceStem;
	const auto original =
		NaNBuster::TestFixtures::LoadBinaryFile((referenceRoot / identity.ReferenceStem / "original.dxbc").string());
	const NaNBuster::DxbcInspector inspector;
	auto originalInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(original));
	CHECK(originalInspected.has_value());
	if (!originalInspected)
	{
		return;
	}
	const auto baseIdentity =
		std::find_if(NaNBuster::LambertShaderIdentities.begin(), NaNBuster::LambertShaderIdentities.end(),
					 [&identity](const auto &candidate) { return candidate.ReferenceStem == identity.ReferenceStem; });
	CHECK(baseIdentity != NaNBuster::LambertShaderIdentities.end());
	if (baseIdentity == NaNBuster::LambertShaderIdentities.end())
	{
		return;
	}
	auto patchIdentity = identity;
	patchIdentity.Identity.ShaderUid = baseIdentity->Identity.ShaderUid;

	for (PatchMode mode : {PatchMode::Lambert, PatchMode::Anisotropic, PatchMode::Combined})
	{
		if (!anisotropic && mode != PatchMode::Lambert)
		{
			continue;
		}
		const std::string_view filename = mode == PatchMode::Lambert	   ? "patched"
										  : mode == PatchMode::Anisotropic ? "anisotropic-clamped"
																		   : "combined";
		const auto corrected =
			NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / (std::string(filename) + ".dxbc")).string());
		const auto expectedAddedBytes = mode == PatchMode::Lambert ? 168 : mode == PatchMode::Anisotropic ? 28 : 196;
		const auto expectedAddedInstructions = mode == PatchMode::Lambert ? 6 : mode == PatchMode::Anisotropic ? 1 : 7;
		const auto expectedAddedTokens = mode == PatchMode::Lambert ? 42 : mode == PatchMode::Anisotropic ? 7 : 49;
		CHECK(corrected.size() == original.size() + expectedAddedBytes);
		CHECK(std::memcmp(original.data() + 4, corrected.data() + 4, 16) != 0);

		auto correctedInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(corrected));
		CHECK(correctedInspected.has_value());
		if (correctedInspected)
		{
			CHECK(correctedInspected->Instructions().size() ==
				  originalInspected->Instructions().size() + expectedAddedInstructions);
			CHECK(correctedInspected->ShaderTokenCount() ==
				  originalInspected->ShaderTokenCount() + expectedAddedTokens);
			CHECK(TempCount(*correctedInspected) ==
				  TempCount(*originalInspected) + (mode == PatchMode::Anisotropic ? 0 : 2));
			CHECK(HasGuard(*correctedInspected) == (mode != PatchMode::Anisotropic));
		}

		Bytes regenerated;
		if (mode == PatchMode::Lambert)
		{
			auto result = NaNBuster::DxbcLambertGuardPatcher{}.Patch(*originalInspected, patchIdentity);
			CHECK(result.has_value());
			if (result)
				regenerated = std::move(result->Bytes);
		}
		else
		{
			auto result = NaNBuster::DxbcAnisotropicSpecularClampPatcher{}.Patch(*originalInspected, patchIdentity,
																				 mode == PatchMode::Combined);
			CHECK(result.has_value());
			if (result)
			{
				regenerated = std::move(result->Bytes);
			}
			else
			{
				std::cerr << identity.Identity.ShaderUid << " mode " << static_cast<int>(mode) << " patch error "
						  << static_cast<int>(result.error().Code) << " at " << result.error().Offset << '\n';
			}
		}
		if (!regenerated.empty())
		{
			CHECK(NaNBuster::DxbcHashWriter{}.Write(regenerated).has_value());
			if (regenerated != corrected)
			{
				std::cerr << identity.Identity.ShaderUid << " mode " << static_cast<int>(mode)
						  << " regenerated payload mismatch\n";
			}
			CHECK(regenerated == corrected);
		}

		const auto generated = GeneratedFor(identity.ReferenceStem, mode);
		CHECK(generated.Size == corrected.size());
		CHECK(std::equal(generated.Data, generated.Data + generated.Size,
						 reinterpret_cast<const std::uint8_t *>(corrected.data())));
		if (mode == PatchMode::Lambert)
		{
			CheckLambertAsmDiff(targetRoot / "original-normalized.asm", targetRoot / "patched.asm");
		}
		else if (mode == PatchMode::Anisotropic)
		{
			CheckAnisotropicAsmDiff(targetRoot / "original-normalized.asm", targetRoot / "anisotropic-clamped.asm");
		}
		else
		{
			CheckAnisotropicAsmDiff(targetRoot / "patched.asm", targetRoot / "combined.asm");
		}
		CheckD3D(device, identity, corrected);
	}
}

void CheckIsotropicAsmDiff(const std::filesystem::path &originalPath, const std::filesystem::path &correctedPath)
{
	const auto original = ReadLines(originalPath);
	const auto corrected = ReadLines(correctedPath);
	CHECK(corrected.size() == original.size() + 18);
	if (corrected.size() != original.size() + 18)
	{
		return;
	}
	constexpr std::array<std::string_view, 13> variance{
		"mad r8.xy, r4.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)",
		"dp2 r4.z, r8.xyxx, r8.xyxx",
		"mad r4.zw, -r4.zzzz, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)",
		"sqrt r4.y, r4.z",
		"mul r8.xy, r4.yyyy, r8.xyxx",
		"mov r8.z, -r4.w",
		"deriv_rtx_fine r5.xyz, r8.xyzx",
		"deriv_rty_fine r6.xyz, r8.xyzx",
		"dp3 r5.w, r5.xyzx, r5.xyzx",
		"dp3 r6.w, r6.xyzx, r6.xyzx",
		"add r5.w, r5.w, r6.w",
		"mul r5.w, r5.w, l(0.250000)",
		"min r8.w, r5.w, l(0.020000)",
	};
	constexpr std::array<std::string_view, 5> exponent{
		"add r6.x, r0.y, l(2.000000)", "div r6.x, l(2.000000), r6.x",  "add_sat r6.x, r6.x, r8.w",
		"div r6.x, l(2.000000), r6.x", "add r0.y, r6.x, l(-2.000000)",
	};
	std::size_t cull = original.size();
	std::size_t exponentAfter = original.size();
	for (std::size_t i = 0; i < original.size(); ++i)
	{
		if (original[i] == "if_nz r2.w")
			cull = i;
		if (original[i] == "mad r0.y, r3.x, l(10.000000), l(1.000000)" && i + 1 < original.size() &&
			original[i + 1] == "exp r0.y, r0.y")
		{
			exponentAfter = i + 2;
		}
	}
	CHECK(cull != original.size());
	CHECK(exponentAfter != original.size());
	if (cull == original.size() || exponentAfter == original.size())
	{
		return;
	}
	std::size_t correctedIndex = 0;
	for (std::size_t i = 0; i < original.size(); ++i)
	{
		if (i == cull)
		{
			for (const auto expected : variance)
			{
				CHECK(corrected[correctedIndex] == expected);
				++correctedIndex;
			}
		}
		if (i == exponentAfter)
		{
			for (const auto expected : exponent)
			{
				CHECK(corrected[correctedIndex] == expected);
				++correctedIndex;
			}
		}
		CHECK(corrected[correctedIndex] == original[i]);
		++correctedIndex;
	}
	CHECK(correctedIndex == corrected.size());
}

void ValidateIsotropic(ID3D11Device *device)
{
	const std::filesystem::path outputRoot = NANBUSTER_AE_SHADER_OUTPUT_ROOT;
	const auto targetRoot = outputRoot / NaNBuster::AEIsotropicSpecularReferenceStem;
	const auto original =
		NaNBuster::TestFixtures::LoadBinaryFile((std::filesystem::path(NANBUSTER_AE_REFERENCE_ROOT) /
												 NaNBuster::AEIsotropicSpecularReferenceStem / "original.dxbc")
													.string());
	const auto corrected = NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "patched.dxbc").string());
	CHECK(original.size() == 6516);
	CHECK(corrected.size() == 7060);
	CHECK(std::memcmp(original.data() + 4, corrected.data() + 4, 16) != 0);
	CHECK(std::equal(NaNBuster::Generated::AE_IsotropicSpecularAA_f4b2da50.begin(),
					 NaNBuster::Generated::AE_IsotropicSpecularAA_f4b2da50.end(),
					 reinterpret_cast<const std::uint8_t *>(corrected.data())));

	const NaNBuster::DxbcInspector inspector;
	auto originalContainer = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(original));
	auto correctedContainer = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(corrected));
	CHECK(originalContainer.has_value());
	CHECK(correctedContainer.has_value());
	if (!originalContainer || !correctedContainer)
	{
		return;
	}
	CHECK(correctedContainer->Instructions().size() == originalContainer->Instructions().size() + 18);
	CHECK(correctedContainer->ShaderTokenCount() == originalContainer->ShaderTokenCount() + 136);
	CHECK(TempCount(*originalContainer) == 9);
	CHECK(TempCount(*correctedContainer) == 9);

	NaNBuster::IsotropicSpecularAAVariantLayout layout{
		.CullTokenOffset = 325,
		.DecodeTokenOffsets = {380, 395, 402, 418, 423, 430},
		.ExponentMadTokenOffset = 1002,
		.ExponentTokenOffset = 1011,
		.ExponentInsertionTokenOffset = 1016,
		.ExponentRegister = 0,
		.ExponentComponent = 1,
	};
	auto patch = NaNBuster::DxbcIsotropicSpecularAAPatcher{}.PatchVariant(
		*originalContainer, NaNBuster::AEIsotropicSpecularTargetIdentity, layout);
	CHECK(patch.has_value());
	if (patch)
	{
		CHECK(NaNBuster::DxbcHashWriter{}.Write(patch->Bytes).has_value());
		CHECK(patch->Bytes == corrected);
	}
	CheckIsotropicAsmDiff(targetRoot / "original-normalized.asm", targetRoot / "patched.asm");
	if (device != nullptr)
	{
		ID3D11PixelShader *raw = nullptr;
		const HRESULT result = device->CreatePixelShader(corrected.data(), corrected.size(), nullptr, &raw);
		std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> shader(raw);
		CHECK(SUCCEEDED(result));
	}
}
} // namespace

auto main() -> int
{
	std::unique_ptr<ID3D11Device, ReleaseDeleter> device;
	HRESULT deviceResult = E_FAIL;
	for (D3D_DRIVER_TYPE type : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP})
	{
		ID3D11Device *rawDevice = nullptr;
		deviceResult =
			D3D11CreateDevice(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &rawDevice, nullptr, nullptr);
		if (SUCCEEDED(deviceResult))
		{
			device.reset(rawDevice);
			break;
		}
	}
	CHECK(SUCCEEDED(deviceResult));

	for (std::size_t index = 0; index < NaNBuster::AELambertShaderIdentities.size(); ++index)
	{
		const bool anisotropic =
			index == 1 || index == 2 || index == 3 || index == 5 || index == 7 || index == 9 || index == 11;
		ValidateTarget(NaNBuster::AELambertShaderIdentities[index], device.get(), anisotropic);
	}
	ValidateIsotropic(device.get());

	if (g_failures != 0)
	{
		std::cerr << "Infrastructure.AEShaderVariantValidation: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.AEShaderVariantValidation: all checks passed (27 deterministic payloads, ASM deltas, "
				 "embedded bytes, D3D creation).\n";
	return EXIT_SUCCESS;
}
