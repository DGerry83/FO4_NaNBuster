#include "Core/TargetShaderMatcher.h"

#include "Core/ShaderIdentityMatcher.h"

namespace NaNBuster
{
	auto TargetShaderMatcher::Matches(ByteView originalBytes) const noexcept -> bool
	{
		return ShaderIdentityMatcher::Matches(originalBytes, OriginalTargetIdentity);
	}
}
