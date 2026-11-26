#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "TestShaderFixtures.h"

#include "bcc0320d-dee924ba-4d26b206-84d74f3f/patched.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "def41f6e-9dbcf63b-4c43cf22-4eea0bb4/patched.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "d75dfae6-af7b9aa5-1a0374d8-3d186619/patched.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "4bfc847c-48cbb854-83d96138-488b9bb1/patched.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "e73d9a26-0c9afe98-f628f975-0e5f8aef/patched.inc"
#include "be21279d-733505d8-43f98a86-09502d65/patched.inc"

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
	struct GeneratedPayload
	{
		const std::uint8_t* Data;
		std::size_t Size;
	};

	template <std::size_t N>
	constexpr auto PayloadOf(const std::array<std::uint8_t, N>& value) -> GeneratedPayload
	{
		return { value.data(), value.size() };
	}

	auto GeneratedFor(std::string_view stem) -> GeneratedPayload
	{
		using namespace NaNBuster::Generated;
		if (stem == "bcc0320d-dee924ba-4d26b206-84d74f3f") return PayloadOf(Lambert_bcc0320d);
		if (stem == "8904fc0f-cb497158-117bc46d-4e1cf466") return PayloadOf(Lambert_8904fc0f);
		if (stem == "4dc83306-024b9074-9c24d67a-fcdee952") return PayloadOf(Lambert_4dc83306);
		if (stem == "325742ce-45bf17d0-f4879324-3637b6aa") return PayloadOf(Lambert_325742ce);
		if (stem == "def41f6e-9dbcf63b-4c43cf22-4eea0bb4") return PayloadOf(Lambert_def41f6e);
		if (stem == "600e22f5-e1a79678-b183a8f0-99de971d") return PayloadOf(Lambert_600e22f5);
		if (stem == "d75dfae6-af7b9aa5-1a0374d8-3d186619") return PayloadOf(Lambert_d75dfae6);
		if (stem == "6b7aa1fd-0175af05-1bf184b7-22dd8846") return PayloadOf(Lambert_6b7aa1fd);
		if (stem == "4bfc847c-48cbb854-83d96138-488b9bb1") return PayloadOf(Lambert_4bfc847c);
		if (stem == "262c37a4-f4966781-271cf8ba-64c06ec1") return PayloadOf(Lambert_262c37a4);
		if (stem == "e73d9a26-0c9afe98-f628f975-0e5f8aef") return PayloadOf(Lambert_e73d9a26);
		if (stem == "be21279d-733505d8-43f98a86-09502d65") return PayloadOf(Lambert_be21279d);
		throw std::runtime_error("unknown generated Lambert payload");
	}

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

	auto TempCount(const NaNBuster::DxbcContainerView& container) -> std::uint32_t
	{
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode == 104) {
				const auto& stream = container.ShaderStream();
				const std::size_t offset = (instruction.TokenOffset + 1) * 4;
				return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset])) |
					(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 1])) << 8) |
					(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 2])) << 16) |
					(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset + 3])) << 24);
			}
		}
		return 0;
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

	void CheckAsmDiff(const std::filesystem::path& originalPath, const std::filesystem::path& correctedPath)
	{
		const auto original = ReadLines(originalPath);
		const auto corrected = ReadLines(correctedPath);
		CHECK(corrected.size() == original.size() + 6);
		if (corrected.size() != original.size() + 6) {
			std::cerr << "ASM line count original=" << original.size() << " corrected=" << corrected.size()
					  << " path=" << originalPath << '\n';
			return;
		}

		std::size_t mad = original.size();
		for (std::size_t i = 0; i < original.size(); ++i) {
			if (original[i].starts_with("mad ") && original[i].contains("4.000000") &&
				original[i].contains("-2.000000") && i + 1 < original.size() && original[i + 1].starts_with("dp2 ")) {
				CHECK(mad == original.size());
				mad = i;
			}
		}
		CHECK(mad != original.size());
		if (mad == original.size()) {
			return;
		}
		const std::size_t guard = mad + 2;
		const std::array<std::string_view, 6> opcodes{ "lt ", "rsq ", "mul ", "movc ", "mul ", "dp2 " };
		for (std::size_t i = 0; i < opcodes.size(); ++i) {
			CHECK(corrected[guard + i].starts_with(opcodes[i]));
		}
		CHECK(corrected[guard + 2].contains("l(1.999000)"));
		CHECK(original[guard - 1].starts_with("dp2 "));
		CHECK(corrected[guard - 1] == original[guard - 1]);
		CHECK(corrected[guard + 6] == original[guard]);

		for (std::size_t i = 0; i < corrected.size(); ++i) {
			if ((i >= guard && i < guard + 6) || corrected[i].starts_with("dcl_temps")) {
				continue;
			}
			const std::size_t expected = i < guard ? i : i - 6;
			if (original[expected].starts_with("dcl_temps")) {
				continue;
			}
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
}

auto main() -> int
{
	const std::filesystem::path referenceRoot = NANBUSTER_LAMBERT_REFERENCE_ROOT;
	const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
	const NaNBuster::DxbcInspector inspector;
	const NaNBuster::DxbcLambertGuardPatcher patcher;
	const NaNBuster::DxbcHashWriter hashWriter;

	for (const auto& identity : NaNBuster::LambertShaderIdentities) {
		const auto original = NaNBuster::TestFixtures::LoadBinaryFile(
			(referenceRoot / identity.ReferenceStem / "original.dxbc").string());
		const auto corrected = NaNBuster::TestFixtures::LoadBinaryFile(
			(outputRoot / identity.ReferenceStem / "patched.dxbc").string());
		CHECK(corrected.size() == original.size() + 168);
		CHECK(std::memcmp(original.data() + 4, corrected.data() + 4, 16) != 0);

		auto originalInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(original));
		auto correctedInspected = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(corrected));
		CHECK(originalInspected.has_value());
		CHECK(correctedInspected.has_value());
		if (!originalInspected || !correctedInspected) {
			continue;
		}
		CHECK(correctedInspected->Instructions().size() == originalInspected->Instructions().size() + 6);
		CHECK(TempCount(*correctedInspected) == TempCount(*originalInspected) + 2);
		CHECK(HasGuard(*correctedInspected));

		auto patched = patcher.Patch(*originalInspected, identity);
		CHECK(patched.has_value());
		if (patched) {
			auto finalized = hashWriter.Write(patched->Bytes);
			CHECK(finalized.has_value());
			CHECK(patched->Bytes == corrected);
		}

		const auto generated = GeneratedFor(identity.ReferenceStem);
		CHECK(generated.Size == corrected.size());
		CHECK(std::equal(generated.Data, generated.Data + generated.Size,
			reinterpret_cast<const std::uint8_t*>(corrected.data())));
		CheckAsmDiff(
			outputRoot / identity.ReferenceStem / "original-normalized.asm",
			outputRoot / identity.ReferenceStem / "patched.asm");
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
		for (const auto& identity : NaNBuster::LambertShaderIdentities) {
			const auto corrected = NaNBuster::TestFixtures::LoadBinaryFile(
				(outputRoot / identity.ReferenceStem / "patched.dxbc").string());
			HRESULT result = E_FAIL;
			if (identity.Stage == NaNBuster::LambertShaderStage::Pixel) {
				ID3D11PixelShader* raw = nullptr;
				result = device->CreatePixelShader(corrected.data(), corrected.size(), nullptr, &raw);
				std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> shader(raw);
			} else {
				ID3D11ComputeShader* raw = nullptr;
				result = device->CreateComputeShader(corrected.data(), corrected.size(), nullptr, &raw);
				std::unique_ptr<ID3D11ComputeShader, ReleaseDeleter> shader(raw);
			}
			if (FAILED(result)) {
				std::cerr << identity.Identity.ShaderUid << " D3D creation failed: 0x" << std::hex << result << std::dec << '\n';
			}
			CHECK(SUCCEEDED(result));
		}
	}

	if (g_failures != 0) {
		std::cerr << "Infrastructure.LambertShaderValidation: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.LambertShaderValidation: all checks passed (12 deterministic payloads, ASM diffs, D3D creation).\n";
	return EXIT_SUCCESS;
}
