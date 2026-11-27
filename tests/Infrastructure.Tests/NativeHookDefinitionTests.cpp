#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/NativeHookDefinition.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace
{
int g_failures = 0;

void Check(bool a_condition, const char *a_expression, const char *a_file, int a_line)
{
	if (!a_condition)
	{
		++g_failures;
		std::cerr << "FAILED: " << a_expression << " (" << a_file << ':' << a_line << ")\n";
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

constexpr std::array kExpectedOgPixel = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xff}, std::byte{0xb0}, std::byte{0x38},
	std::byte{0x04}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24}, std::byte{0x60},
	std::byte{0x48}, std::byte{0x8d}, std::byte{0x47}, std::byte{0x08}, std::byte{0x4c}, std::byte{0x8b},
	std::byte{0x11}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48}, std::byte{0x8b},
	std::byte{0xd3}, std::byte{0x48}, std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20},
	std::byte{0x41}, std::byte{0xff}, std::byte{0x52}, std::byte{0x78},
};

constexpr std::array kExpectedAePixel = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xa3}, std::byte{0x34}, std::byte{0x0c},
	std::byte{0x02}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x44}, std::byte{0x8b},
	std::byte{0x44}, std::byte{0x24}, std::byte{0x60}, std::byte{0x48}, std::byte{0x8b}, std::byte{0xd3},
	std::byte{0x48}, std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x4c},
	std::byte{0x8b}, std::byte{0x11}, std::byte{0x41}, std::byte{0xff}, std::byte{0x52}, std::byte{0x78},
};

constexpr std::array kExpectedOgCompute = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xa7}, std::byte{0xae}, std::byte{0x38},
	std::byte{0x04}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24}, std::byte{0x58},
	std::byte{0x48}, std::byte{0x8d}, std::byte{0x43}, std::byte{0x08}, std::byte{0x4c}, std::byte{0x8b},
	std::byte{0x11}, std::byte{0x48}, std::byte{0x8d}, std::byte{0x93}, std::byte{0x88}, std::byte{0x00},
	std::byte{0x00}, std::byte{0x00}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48},
	std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x41}, std::byte{0xff},
	std::byte{0x92}, std::byte{0x90}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
};

constexpr std::array kExpectedAeCompute = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0x5d}, std::byte{0x32}, std::byte{0x0c},
	std::byte{0x02}, std::byte{0x48}, std::byte{0x8d}, std::byte{0x93}, std::byte{0x88}, std::byte{0x00},
	std::byte{0x00}, std::byte{0x00}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24},
	std::byte{0x58}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48}, std::byte{0x89},
	std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x4c}, std::byte{0x8b}, std::byte{0x11},
	std::byte{0x41}, std::byte{0xff}, std::byte{0x92}, std::byte{0x90}, std::byte{0x00}, std::byte{0x00},
	std::byte{0x00},
};

template <std::size_t N>
void CheckDefinition(const NaNBuster::NativeHookDefinition *a_definition, std::uint32_t a_owner,
					 std::ptrdiff_t a_siteOffset, std::uint32_t a_device, const std::array<std::byte, N> &a_expected)
{
	CHECK(a_definition != nullptr);
	if (a_definition == nullptr)
	{
		return;
	}
	CHECK(a_definition->OwnerRelocationID == a_owner);
	CHECK(a_definition->SiteOffset == a_siteOffset);
	CHECK(a_definition->DeviceGlobalRelocationID == a_device);
	CHECK(a_definition->DetourSize == 7);
	CHECK(a_definition->ExpectedSiteBytes.size() == N);
	CHECK(a_definition->DetourSize <= a_definition->ExpectedSiteBytes.size());
	CHECK(std::equal(a_definition->ExpectedSiteBytes.begin(), a_definition->ExpectedSiteBytes.end(), a_expected.begin(),
					 a_expected.end()));
}
} // namespace

auto main() -> int
{
	using F4SE::RuntimeType;

	const auto *ogPixel = NaNBuster::GetPixelShaderHookDefinition(RuntimeType::kOG);
	const auto *aePixel = NaNBuster::GetPixelShaderHookDefinition(RuntimeType::kAE);
	const auto *ogCompute = NaNBuster::GetComputeShaderHookDefinition(RuntimeType::kOG);
	const auto *aeCompute = NaNBuster::GetComputeShaderHookDefinition(RuntimeType::kAE);

	CheckDefinition(ogPixel, 957517, 0x142, 1454788, kExpectedOgPixel);
	CheckDefinition(aePixel, 2276951, 0x12e, 2704430, kExpectedAePixel);
	CheckDefinition(ogCompute, 957517, 0x39a, 1454788, kExpectedOgCompute);
	CheckDefinition(aeCompute, 2276955, 0x114, 2704430, kExpectedAeCompute);

	CHECK(NaNBuster::GetPixelShaderHookDefinition(RuntimeType::kOG) == ogPixel);
	CHECK(NaNBuster::GetPixelShaderHookDefinition(RuntimeType::kAE) == aePixel);
	CHECK(NaNBuster::GetComputeShaderHookDefinition(RuntimeType::kOG) == ogCompute);
	CHECK(NaNBuster::GetComputeShaderHookDefinition(RuntimeType::kAE) == aeCompute);
	CHECK(ogPixel != aePixel);
	CHECK(ogCompute != aeCompute);

	constexpr std::array kUnsupported = {
		RuntimeType::kNone,
		RuntimeType::kVR,
		RuntimeType::kNG,
		static_cast<RuntimeType>(999),
	};
	for (const auto runtime : kUnsupported)
	{
		CHECK(NaNBuster::GetPixelShaderHookDefinition(runtime) == nullptr);
		CHECK(NaNBuster::GetComputeShaderHookDefinition(runtime) == nullptr);
	}

	if (g_failures != 0)
	{
		std::cerr << "Infrastructure.NativeHookDefinition: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.NativeHookDefinition: all checks passed (exact OG/AE definitions, static lifetime, "
				 "unsupported runtime null).\n";
	return EXIT_SUCCESS;
}
