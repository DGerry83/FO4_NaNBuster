#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace NaNBuster
{
	using ByteView = std::span<const std::byte>;

	struct ShaderIdentity
	{
		std::string_view ShaderUid;
		std::uint32_t BytecodeSize;
		std::array<std::byte, 16> EmbeddedHash;
		std::uint32_t GfxBoosterHash;
		std::uint32_t AssemblyHash;
	};

	// On-disk byte sequence at DXBC offset 4; display form 82f7e159-8db6b7a4-b742fff3-b11324e9.
	inline constexpr ShaderIdentity OriginalTargetIdentity{
		.ShaderUid = "PS49B6A21CI0O1",
		.BytecodeSize = 1612u,
		.EmbeddedHash = {
			std::byte{ 0x59 }, std::byte{ 0xe1 }, std::byte{ 0xf7 }, std::byte{ 0x82 },
			std::byte{ 0xa4 }, std::byte{ 0xb7 }, std::byte{ 0xb6 }, std::byte{ 0x8d },
			std::byte{ 0xf3 }, std::byte{ 0xff }, std::byte{ 0x42 }, std::byte{ 0xb7 },
			std::byte{ 0xe9 }, std::byte{ 0x24 }, std::byte{ 0x13 }, std::byte{ 0xb1 },
		},
		.GfxBoosterHash = 0xF823EE3Fu,
		.AssemblyHash = 0x49B6A21Cu,
	};
}
