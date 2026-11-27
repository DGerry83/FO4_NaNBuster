#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace F4SE
{
enum class RuntimeType : std::int32_t;
}

namespace NaNBuster
{
struct NativeHookDefinition
{
	std::uint32_t OwnerRelocationID;
	std::ptrdiff_t SiteOffset;
	std::uint32_t DeviceGlobalRelocationID;
	std::span<const std::byte> ExpectedSiteBytes;
	std::size_t DetourSize;
};

[[nodiscard]] auto GetPixelShaderHookDefinition(F4SE::RuntimeType a_runtime) noexcept -> const NativeHookDefinition *;
[[nodiscard]] auto GetComputeShaderHookDefinition(F4SE::RuntimeType a_runtime) noexcept -> const NativeHookDefinition *;
} // namespace NaNBuster
