#pragma once

#include "Application/IShaderPayload.h"
#include "Application/ShaderSelection.h"
#include "Core/TargetShaderMatcher.h"

namespace NaNBuster
{
	// Match-and-substitute use case: decides, for one original shader creation,
	// whether the corrected payload replaces the original bytecode. Holds
	// non-owning references to the matcher and payload, whose lifetimes must
	// exceed the service's. Select performs no allocation and never throws; on
	// a nonmatch it returns the caller's original view as NotTarget without
	// querying the payload at all.
	class ShaderPatchService
	{
	public:
		ShaderPatchService(const TargetShaderMatcher& matcher, const IShaderPayload& payload) noexcept;

		auto Select(ByteView originalBytes) const noexcept -> ShaderSelection;

	private:
		const TargetShaderMatcher& m_matcher;
		const IShaderPayload& m_payload;
	};
}
