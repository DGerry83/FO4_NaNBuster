#pragma once

#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	class ShaderIdentityMatcher final
	{
	public:
		[[nodiscard]] static auto Matches(ByteView originalBytes, const ShaderIdentity& identity) noexcept -> bool;
	};
}
