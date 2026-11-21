#pragma once

#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	// Exact-match predicate for the approved original target DXBC.
	// Matches returns true only when originalBytes has exactly the target
	// bytecode size and the raw 16-byte embedded hash at DXBC offset 4
	// equals OriginalTargetIdentity.EmbeddedHash. The size is checked before
	// the hash bytes are read, so shorter or longer views are rejected
	// without touching out-of-range storage. No UID, prefix, or magic-byte
	// matching. Standard-library only; allocation-free; never throws.
	class TargetShaderMatcher
	{
	public:
		auto Matches(ByteView originalBytes) const noexcept -> bool;
	};
}
