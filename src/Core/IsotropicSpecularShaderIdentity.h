#pragma once

#include "Core/ShaderIdentity.h"

#include <string_view>

namespace NaNBuster
{
	inline constexpr ShaderIdentity IsotropicSpecularTargetIdentity{
		.ShaderUid = "f4b2da50-c279fcd9-bd60ba4f-6a804632",
		.BytecodeSize = 6528u,
		.EmbeddedHash = {
			std::byte{ 0x50 }, std::byte{ 0xda }, std::byte{ 0xb2 }, std::byte{ 0xf4 },
			std::byte{ 0xd9 }, std::byte{ 0xfc }, std::byte{ 0x79 }, std::byte{ 0xc2 },
			std::byte{ 0x4f }, std::byte{ 0xba }, std::byte{ 0x60 }, std::byte{ 0xbd },
			std::byte{ 0x32 }, std::byte{ 0x46 }, std::byte{ 0x80 }, std::byte{ 0x6a },
		},
		.GfxBoosterHash = 0,
		.AssemblyHash = 0,
	};

	inline constexpr std::string_view IsotropicSpecularReferenceStem = "f4b2da50-c279fcd9-bd60ba4f-6a804632";
}
