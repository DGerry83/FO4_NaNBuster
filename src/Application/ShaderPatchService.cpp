#include "Application/ShaderPatchService.h"

namespace NaNBuster
{
	ShaderPatchService::ShaderPatchService(const TargetShaderMatcher& matcher, const IShaderPayload& payload) noexcept
		: m_matcher(matcher)
		, m_payload(payload)
	{
	}

	auto ShaderPatchService::Select(ByteView originalBytes) const noexcept -> ShaderSelection
	{
		if (!m_matcher.Matches(originalBytes)) {
			return ShaderSelection{ originalBytes, false, PatchOutcome::NotTarget };
		}

		return ShaderSelection{ m_payload.GetCorrectedBytecode(), true, PatchOutcome::Patched };
	}
}
