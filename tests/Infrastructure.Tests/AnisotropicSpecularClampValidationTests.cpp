#include "Core/DxbcAnisotropicSpecularClampPatcher.h"
#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "TestShaderFixtures.h"

#include "8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/anisotropic-clamped.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/combined.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/anisotropic-clamped.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/combined.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/anisotropic-clamped.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/combined.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/anisotropic-clamped.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/combined.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/anisotropic-clamped.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/combined.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/anisotropic-clamped.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/combined.inc"
#include "be21279d-733505d8-43f98a86-09502d65/patched.inc"
#include "be21279d-733505d8-43f98a86-09502d65/anisotropic-clamped.inc"
#include "be21279d-733505d8-43f98a86-09502d65/combined.inc"

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
#include <vector>

#include <d3d11.h>

#ifndef NANBUSTER_LAMBERT_REFERENCE_ROOT
#	error "NANBUSTER_LAMBERT_REFERENCE_ROOT is required."
#endif
#ifndef NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT
#	error "NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT is required."
#endif

namespace
{
	using Bytes = std::vector<std::byte>;
	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

	auto ReadLines(const std::filesystem::path& path) -> std::vector<std::string>
	{
		std::ifstream stream(path);
		if (!stream.is_open()) {
			throw std::runtime_error("failed to open ASM: " + path.string());
		}
		std::vector<std::string> lines;
		for (std::string line; std::getline(stream, line);) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			const auto first = line.find_first_not_of(" \t");
			if (first == std::string::npos || line.starts_with("Shader hash") || line.starts_with("//")) {
				continue;
			}
			const auto last = line.find_last_not_of(" \t");
			lines.push_back(line.substr(first, last - first + 1));
		}
		return lines;
	}

	auto Token(const NaNBuster::DxbcContainerView& container, std::size_t offset) -> std::uint32_t
	{
		const auto& stream = container.ShaderStream();
		const std::size_t at = offset * 4;
		return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at])) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 1])) << 8) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 2])) << 16) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 3])) << 24);
	}

	auto TempCount(const NaNBuster::DxbcContainerView& container) -> std::uint32_t
	{
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode == 104) {
				return Token(container, instruction.TokenOffset + 1);
			}
		}
		return 0;
	}

	bool HasClamp(const NaNBuster::DxbcContainerView& container)
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 1 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 51 && instructions[i + 1].Opcode == 47 &&
				Token(container, instructions[i].TokenOffset + 6) == 0x3F800000u) {
				return true;
			}
		}
		return false;
	}

	bool HasGuard(const NaNBuster::DxbcContainerView& container)
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 5 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 49 && instructions[i + 1].Opcode == 68 &&
				instructions[i + 2].Opcode == 56 && instructions[i + 3].Opcode == 55 &&
				instructions[i + 4].Opcode == 56 && instructions[i + 5].Opcode == 15) {
				return true;
			}
		}
		return false;
	}

	void CheckAsmDiff(
		const std::filesystem::path& originalPath,
		const std::filesystem::path& correctedPath,
		std::string_view registerName)
	{
		const auto original = ReadLines(originalPath);
		const auto corrected = ReadLines(correctedPath);
		CHECK(corrected.size() == original.size() + 1);
		if (corrected.size() != original.size() + 1) {
			return;
		}

		std::size_t anchor = original.size();
		for (std::size_t i = 0; i + 4 < original.size(); ++i) {
			if (original[i].starts_with("max ") && original[i + 1].starts_with("log ") &&
				original[i + 2].starts_with("mul ") && original[i + 3].starts_with("exp ") &&
				original[i + 4].starts_with("mul ")) {
				CHECK(anchor == original.size());
				anchor = i;
			}
		}
		CHECK(anchor != original.size());
		if (anchor == original.size()) {
			return;
		}
		CHECK(corrected[anchor] == original[anchor]);
		CHECK(corrected[anchor + 1] == "min " + std::string(registerName) + ", " + std::string(registerName) + ", l(1.000000)");
		for (std::size_t i = 0; i < corrected.size(); ++i) {
			if (i == anchor + 1) {
				continue;
			}
			const std::size_t expected = i <= anchor ? i : i - 1;
			CHECK(corrected[i] == original[expected]);
		}
	}

	struct ReleaseDeleter
	{
		template <typename T>
		void operator()(T* pointer) const
		{
			if (pointer != nullptr) {
				pointer->Release();
			}
		}
	};

	template <std::size_t N>
	void CheckEmbedded(const std::array<std::uint8_t, N>& embedded, const Bytes& corrected)
	{
		CHECK(embedded.size() == corrected.size());
		CHECK(std::equal(embedded.begin(), embedded.end(), reinterpret_cast<const std::uint8_t*>(corrected.data())));
	}
}

void ValidateTarget(const NaNBuster::LambertShaderIdentity& identity)
{
	const std::filesystem::path referenceRoot = NANBUSTER_LAMBERT_REFERENCE_ROOT;
	const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
	const auto targetRoot = outputRoot / identity.ReferenceStem;
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile((referenceRoot / identity.ReferenceStem / "original.dxbc").string());
	const auto lambert = NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "patched.dxbc").string());
	const auto clamped = NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "anisotropic-clamped.dxbc").string());
	const auto combined = NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "combined.dxbc").string());
	CHECK(lambert.size() == original.size() + 168);
	CHECK(clamped.size() == original.size() + 28);
	CHECK(combined.size() == original.size() + 196);
	CHECK(std::memcmp(original.data() + 4, clamped.data() + 4, 16) != 0);
	CHECK(std::memcmp(original.data() + 4, combined.data() + 4, 16) != 0);
	if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[1].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_8904fc0f, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_8904fc0f, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_8904fc0f, combined);
	} else if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[2].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_4dc83306, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_4dc83306, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_4dc83306, combined);
	} else if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[3].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_325742ce, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_325742ce, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_325742ce, combined);
	} else if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[5].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_600e22f5, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_600e22f5, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_600e22f5, combined);
	} else if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[7].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_6b7aa1fd, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_6b7aa1fd, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_6b7aa1fd, combined);
	} else if (identity.Identity.ShaderUid == NaNBuster::LambertShaderIdentities[9].Identity.ShaderUid) {
		CheckEmbedded(NaNBuster::Generated::Lambert_262c37a4, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_262c37a4, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_262c37a4, combined);
	} else {
		CheckEmbedded(NaNBuster::Generated::Lambert_be21279d, lambert);
		CheckEmbedded(NaNBuster::Generated::Anisotropic_be21279d, clamped);
		CheckEmbedded(NaNBuster::Generated::Combined_be21279d, combined);
	}

	const NaNBuster::DxbcInspector inspector;
	auto originalInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(original));
	auto clampedInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(clamped));
	auto combinedInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(combined));
	CHECK(originalInspected.has_value());
	CHECK(clampedInspected.has_value());
	CHECK(combinedInspected.has_value());
	if (originalInspected && clampedInspected && combinedInspected) {
		CHECK(clampedInspected->Instructions().size() == originalInspected->Instructions().size() + 1);
		CHECK(combinedInspected->Instructions().size() == originalInspected->Instructions().size() + 7);
		CHECK(TempCount(*clampedInspected) == TempCount(*originalInspected));
		CHECK(TempCount(*combinedInspected) == TempCount(*originalInspected) + 2);
		CHECK(HasClamp(*clampedInspected));
		CHECK(!HasGuard(*clampedInspected));
		CHECK(HasClamp(*combinedInspected));
		CHECK(HasGuard(*combinedInspected));

		const NaNBuster::DxbcHashWriter hashWriter;
		auto lambertPatch = NaNBuster::DxbcLambertGuardPatcher{}.Patch(*originalInspected, identity);
		auto clampPatch = NaNBuster::DxbcAnisotropicSpecularClampPatcher{}.Patch(*originalInspected, identity);
		auto combinedPatch = NaNBuster::DxbcAnisotropicSpecularClampPatcher{}.Patch(*originalInspected, identity, true);
		CHECK(lambertPatch.has_value());
		CHECK(clampPatch.has_value());
		CHECK(combinedPatch.has_value());
		if (lambertPatch && clampPatch && combinedPatch) {
			CHECK(hashWriter.Write(lambertPatch->Bytes).has_value());
			CHECK(hashWriter.Write(clampPatch->Bytes).has_value());
			CHECK(hashWriter.Write(combinedPatch->Bytes).has_value());
			CHECK(lambertPatch->Bytes == lambert);
			CHECK(clampPatch->Bytes == clamped);
			CHECK(combinedPatch->Bytes == combined);
		}
	}
	const auto registerName = identity.Stage == NaNBuster::LambertShaderStage::Compute ? "r19.x" :
		(&identity == &NaNBuster::LambertShaderIdentities[7] || &identity == &NaNBuster::LambertShaderIdentities[9]) ? "r3.w" : "r3.z";
	CheckAsmDiff(targetRoot / "original-normalized.asm", targetRoot / "anisotropic-clamped.asm", registerName);
	CheckAsmDiff(targetRoot / "patched.asm", targetRoot / "combined.asm", registerName);
}

auto main() -> int
{
	for (const auto index : { 1, 2, 3, 5, 7, 9, 11 }) {
		ValidateTarget(NaNBuster::LambertShaderIdentities[index]);
	}

	std::unique_ptr<ID3D11Device, ReleaseDeleter> device;
	HRESULT deviceResult = E_FAIL;
	for (D3D_DRIVER_TYPE type : { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP }) {
		ID3D11Device* rawDevice = nullptr;
		deviceResult = D3D11CreateDevice(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &rawDevice, nullptr, nullptr);
		if (SUCCEEDED(deviceResult)) {
			device.reset(rawDevice);
			break;
		}
	}
	CHECK(SUCCEEDED(deviceResult));
	if (device) {
		const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
		for (const auto& identity : NaNBuster::LambertShaderIdentities) {
			if (&identity != &NaNBuster::LambertShaderIdentities[1] && &identity != &NaNBuster::LambertShaderIdentities[2] &&
				&identity != &NaNBuster::LambertShaderIdentities[3] && &identity != &NaNBuster::LambertShaderIdentities[5] &&
				&identity != &NaNBuster::LambertShaderIdentities[7] && &identity != &NaNBuster::LambertShaderIdentities[9] &&
				&identity != &NaNBuster::LambertShaderIdentities[11]) {
				continue;
			}
			const auto targetRoot = outputRoot / identity.ReferenceStem;
			for (const auto* filename : { "anisotropic-clamped.dxbc", "combined.dxbc" }) {
				const auto bytes = NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / filename).string());
				HRESULT result = E_FAIL;
				if (identity.Stage == NaNBuster::LambertShaderStage::Pixel) {
					ID3D11PixelShader* raw = nullptr;
					result = device->CreatePixelShader(bytes.data(), bytes.size(), nullptr, &raw);
					std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> shader(raw);
				} else {
					ID3D11ComputeShader* raw = nullptr;
					result = device->CreateComputeShader(bytes.data(), bytes.size(), nullptr, &raw);
					std::unique_ptr<ID3D11ComputeShader, ReleaseDeleter> shader(raw);
				}
				if (FAILED(result)) {
					std::cerr << identity.Identity.ShaderUid << " anisotropic payload D3D creation failed: 0x"
							  << std::hex << result << std::dec << '\n';
				}
				CHECK(SUCCEEDED(result));
			}
		}
	}

	if (g_failures != 0) {
		std::cerr << "Infrastructure.AnisotropicSpecularClampValidation: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.AnisotropicSpecularClampValidation: all checks passed (7 targets, 21 payloads, ASM diffs, D3D creation).\n";
	return EXIT_SUCCESS;
}
