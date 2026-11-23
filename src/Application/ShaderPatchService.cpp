#include "Application/ShaderPatchService.h"

#include "Core/ShaderIdentityMatcher.h"

namespace NaNBuster
{
	ShaderPatchService::ShaderPatchService(const TargetShaderMatcher& matcher, const IShaderPayload& payload) noexcept
		: m_matcher(&matcher)
		, m_payload(&payload)
		, m_registry(nullptr)
		, m_policy(FixPolicy::AllEnabled())
	{
	}

	ShaderPatchService::ShaderPatchService(const IShaderPatchRegistry& registry, FixPolicy policy) noexcept
		: m_matcher(nullptr)
		, m_payload(nullptr)
		, m_registry(&registry)
		, m_policy(policy)
	{
	}

	auto ShaderPatchService::Select(ByteView originalBytes) const noexcept -> ShaderSelection
	{
		if (m_registry != nullptr) {
			for (const auto& entry : m_registry->Entries()) {
				if (!m_policy.IsEnabled(entry.Fix)) {
					continue;
				}
				if (ShaderIdentityMatcher::Matches(originalBytes, *entry.SourceIdentity)) {
					return ShaderSelection{ entry.CorrectedBytecode, true, PatchOutcome::Patched, entry.SourceIdentity };
				}
			}
			return ShaderSelection{ originalBytes, false, PatchOutcome::NotTarget };
		}

		if (!m_matcher->Matches(originalBytes)) {
			return ShaderSelection{ originalBytes, false, PatchOutcome::NotTarget };
		}

		return ShaderSelection{ m_payload->GetCorrectedBytecode(), true, PatchOutcome::Patched, &OriginalTargetIdentity };
	}
}
