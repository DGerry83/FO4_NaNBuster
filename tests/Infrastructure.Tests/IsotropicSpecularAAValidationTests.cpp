#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcIsotropicSpecularAAPatcher.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "TestShaderFixtures.h"

#include "patched.inc"

#include <algorithm>
#include <array>
#include <cstddef>
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

#ifndef NANBUSTER_ISOTROPIC_ORIGINAL_PATH
#	error "NANBUSTER_ISOTROPIC_ORIGINAL_PATH is required."
#endif
#ifndef NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR
#	error "NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR is required."
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

	auto TempCount(const NaNBuster::DxbcContainerView& container) -> std::uint32_t
	{
		const auto& stream = container.ShaderStream();
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode != 104) {
				continue;
			}
			const std::size_t at = (instruction.TokenOffset + 1) * 4;
			return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at])) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 1])) << 8) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 2])) << 16) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 3])) << 24);
		}
		return 0;
	}

	void CheckAsmDiff(const std::filesystem::path& originalPath, const std::filesystem::path& correctedPath)
	{
		const auto original = ReadLines(originalPath);
		const auto corrected = ReadLines(correctedPath);
		CHECK(corrected.size() == original.size() + 18);
		if (corrected.size() != original.size() + 18) {
			return;
		}

		constexpr std::array<std::string_view, 13> variance{
			"mad r8.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)",
			"dp2 r4.z, r8.xyxx, r8.xyxx",
			"mad r4.yz, -r4.zzzz, l(0.000000, 0.250000, 0.500000, 0.000000), l(0.000000, 1.000000, 1.000000, 0.000000)",
			"sqrt r4.y, r4.y",
			"mul r8.xy, r4.yyyy, r8.xyxx",
			"mov r8.z, -r4.z",
			"deriv_rtx_fine r5.xyz, r8.xyzx",
			"deriv_rty_fine r6.xyz, r8.xyzx",
			"dp3 r5.w, r5.xyzx, r5.xyzx",
			"dp3 r6.w, r6.xyzx, r6.xyzx",
			"add r5.w, r5.w, r6.w",
			"mul r5.w, r5.w, l(0.250000)",
			"min r8.w, r5.w, l(0.020000)",
		};
		constexpr std::array<std::string_view, 5> exponent{
			"add r6.x, r3.x, l(2.000000)",
			"div r6.x, l(2.000000), r6.x",
			"add_sat r6.x, r6.x, r8.w",
			"div r6.x, l(2.000000), r6.x",
			"add r3.x, r6.x, l(-2.000000)",
		};

		std::size_t cull = original.size();
		std::size_t exponentAfter = original.size();
		for (std::size_t i = 0; i < original.size(); ++i) {
			if (original[i] == "if_nz r2.w") {
				cull = i;
			}
			if (original[i] == "exp r3.x, r3.x") {
				exponentAfter = i + 1;
			}
		}
		CHECK(cull != original.size());
		CHECK(exponentAfter != original.size());
		if (cull == original.size() || exponentAfter == original.size()) {
			return;
		}

		std::size_t correctedIndex = 0;
		for (std::size_t i = 0; i < original.size(); ++i) {
			if (i == cull) {
				for (const auto expected : variance) {
					CHECK(corrected[correctedIndex] == expected);
					++correctedIndex;
				}
			}
			if (i == exponentAfter) {
				for (const auto expected : exponent) {
					CHECK(corrected[correctedIndex] == expected);
					++correctedIndex;
				}
			}
			CHECK(corrected[correctedIndex] == original[i]);
			++correctedIndex;
		}
		CHECK(correctedIndex == corrected.size());
	}

	struct ReleaseDeleter
	{
		template <typename T>
		void operator()(T* pointer) const
		{
			if (pointer) {
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

auto main() -> int
{
	const std::filesystem::path outputRoot = NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR;
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH);
	const auto corrected = NaNBuster::TestFixtures::LoadBinaryFile((outputRoot / "patched.dxbc").string());
	CHECK(original.size() == 6528);
	CHECK(corrected.size() == 7072);
	CHECK(std::memcmp(original.data() + 4, corrected.data() + 4, 16) != 0);
	CheckEmbedded(NaNBuster::Generated::IsotropicSpecularAA_f4b2da50, corrected);

	const NaNBuster::DxbcInspector inspector;
	auto originalContainer = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(original));
	auto correctedContainer = inspector.Inspect(NaNBuster::TestFixtures::AsByteView(corrected));
	CHECK(originalContainer.has_value());
	CHECK(correctedContainer.has_value());
	if (!originalContainer || !correctedContainer) {
		return EXIT_FAILURE;
	}
	CHECK(correctedContainer->Instructions().size() == originalContainer->Instructions().size() + 18);
	CHECK(correctedContainer->ShaderTokenCount() == originalContainer->ShaderTokenCount() + 136);
	CHECK(TempCount(*originalContainer) == 9);
	CHECK(TempCount(*correctedContainer) == 9);

	auto patch = NaNBuster::DxbcIsotropicSpecularAAPatcher{}.Patch(*originalContainer);
	CHECK(patch.has_value());
	if (patch) {
		CHECK(NaNBuster::DxbcHashWriter{}.Write(patch->Bytes).has_value());
		CHECK(patch->Bytes == corrected);
	}
	CheckAsmDiff(outputRoot / "original-normalized.asm", outputRoot / "patched.asm");

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
		ID3D11PixelShader* raw = nullptr;
		const HRESULT result = device->CreatePixelShader(corrected.data(), corrected.size(), nullptr, &raw);
		std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> shader(raw);
		if (FAILED(result)) {
			std::cerr << "isotropic payload D3D creation failed: 0x" << std::hex << result << std::dec << '\n';
		}
		CHECK(SUCCEEDED(result));
	}

	if (g_failures != 0) {
		std::cerr << "Infrastructure.IsotropicSpecularAAValidation: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.IsotropicSpecularAAValidation: all checks passed (7072-byte generated/embedded payload, exact ASM delta, D3D creation).\n";
	return EXIT_SUCCESS;
}
