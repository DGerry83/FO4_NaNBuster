#include "Core/ShaderIdentityMatcher.h"

#include <cstddef>
#include <cstring>

namespace NaNBuster
{
	namespace
	{
		constexpr std::size_t kDxbcEmbeddedHashOffset = 4;
	}

	auto ShaderIdentityMatcher::Matches(ByteView originalBytes, const ShaderIdentity& identity) noexcept -> bool
	{
		if (originalBytes.size() != identity.BytecodeSize) {
			return false;
		}

		return std::memcmp(
				   originalBytes.data() + kDxbcEmbeddedHashOffset,
				   identity.EmbeddedHash.data(),
				   identity.EmbeddedHash.size()) == 0;
	}
}
