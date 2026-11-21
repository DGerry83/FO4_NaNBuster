#include "Core/TargetShaderMatcher.h"

#include <cstddef>
#include <cstring>

namespace
{
	// Raw embedded-hash field location in a DXBC container header.
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;
}

namespace NaNBuster
{
	auto TargetShaderMatcher::Matches(ByteView originalBytes) const noexcept -> bool
	{
		if (originalBytes.size() != OriginalTargetIdentity.BytecodeSize) {
			return false;
		}

		return std::memcmp(
				   originalBytes.data() + DxbcEmbeddedHashOffset,
				   OriginalTargetIdentity.EmbeddedHash.data(),
				   OriginalTargetIdentity.EmbeddedHash.size()) == 0;
	}
}
