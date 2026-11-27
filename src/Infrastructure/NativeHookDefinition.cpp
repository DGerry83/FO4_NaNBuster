#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/NativeHookDefinition.h"

#include <array>

namespace NaNBuster
{
namespace
{
constexpr std::array kOgPixelBytes = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xff}, std::byte{0xb0}, std::byte{0x38},
	std::byte{0x04}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24}, std::byte{0x60},
	std::byte{0x48}, std::byte{0x8d}, std::byte{0x47}, std::byte{0x08}, std::byte{0x4c}, std::byte{0x8b},
	std::byte{0x11}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48}, std::byte{0x8b},
	std::byte{0xd3}, std::byte{0x48}, std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20},
	std::byte{0x41}, std::byte{0xff}, std::byte{0x52}, std::byte{0x78},
};

constexpr std::array kAePixelBytes = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xa3}, std::byte{0x34}, std::byte{0x0c},
	std::byte{0x02}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x44}, std::byte{0x8b},
	std::byte{0x44}, std::byte{0x24}, std::byte{0x60}, std::byte{0x48}, std::byte{0x8b}, std::byte{0xd3},
	std::byte{0x48}, std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x4c},
	std::byte{0x8b}, std::byte{0x11}, std::byte{0x41}, std::byte{0xff}, std::byte{0x52}, std::byte{0x78},
};

constexpr std::array kOgComputeBytes = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0xa7}, std::byte{0xae}, std::byte{0x38},
	std::byte{0x04}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24}, std::byte{0x58},
	std::byte{0x48}, std::byte{0x8d}, std::byte{0x43}, std::byte{0x08}, std::byte{0x4c}, std::byte{0x8b},
	std::byte{0x11}, std::byte{0x48}, std::byte{0x8d}, std::byte{0x93}, std::byte{0x88}, std::byte{0x00},
	std::byte{0x00}, std::byte{0x00}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48},
	std::byte{0x89}, std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x41}, std::byte{0xff},
	std::byte{0x92}, std::byte{0x90}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
};

constexpr std::array kAeComputeBytes = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0x5d}, std::byte{0x32}, std::byte{0x0c},
	std::byte{0x02}, std::byte{0x48}, std::byte{0x8d}, std::byte{0x93}, std::byte{0x88}, std::byte{0x00},
	std::byte{0x00}, std::byte{0x00}, std::byte{0x44}, std::byte{0x8b}, std::byte{0x44}, std::byte{0x24},
	std::byte{0x58}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48}, std::byte{0x89},
	std::byte{0x44}, std::byte{0x24}, std::byte{0x20}, std::byte{0x4c}, std::byte{0x8b}, std::byte{0x11},
	std::byte{0x41}, std::byte{0xff}, std::byte{0x92}, std::byte{0x90}, std::byte{0x00}, std::byte{0x00},
	std::byte{0x00},
};

constexpr NativeHookDefinition kOgPixelDefinition{
	.OwnerRelocationID = 957517,
	.SiteOffset = 0x142,
	.DeviceGlobalRelocationID = 1454788,
	.ExpectedSiteBytes = kOgPixelBytes,
	.DetourSize = 7,
};

constexpr NativeHookDefinition kAePixelDefinition{
	.OwnerRelocationID = 2276951,
	.SiteOffset = 0x12e,
	.DeviceGlobalRelocationID = 2704430,
	.ExpectedSiteBytes = kAePixelBytes,
	.DetourSize = 7,
};

constexpr NativeHookDefinition kOgComputeDefinition{
	.OwnerRelocationID = 957517,
	.SiteOffset = 0x39a,
	.DeviceGlobalRelocationID = 1454788,
	.ExpectedSiteBytes = kOgComputeBytes,
	.DetourSize = 7,
};

constexpr NativeHookDefinition kAeComputeDefinition{
	.OwnerRelocationID = 2276955,
	.SiteOffset = 0x114,
	.DeviceGlobalRelocationID = 2704430,
	.ExpectedSiteBytes = kAeComputeBytes,
	.DetourSize = 7,
};
} // namespace

auto GetPixelShaderHookDefinition(F4SE::RuntimeType a_runtime) noexcept -> const NativeHookDefinition *
{
	switch (a_runtime)
	{
	case F4SE::RuntimeType::kOG:
		return &kOgPixelDefinition;
	case F4SE::RuntimeType::kAE:
		return &kAePixelDefinition;
	default:
		return nullptr;
	}
}

auto GetComputeShaderHookDefinition(F4SE::RuntimeType a_runtime) noexcept -> const NativeHookDefinition *
{
	switch (a_runtime)
	{
	case F4SE::RuntimeType::kOG:
		return &kOgComputeDefinition;
	case F4SE::RuntimeType::kAE:
		return &kAeComputeDefinition;
	default:
		return nullptr;
	}
}
} // namespace NaNBuster
