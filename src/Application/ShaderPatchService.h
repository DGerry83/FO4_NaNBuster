#pragma once

#include "Application/IShaderPayload.h"
#include "Application/ShaderPatchRegistry.h"
#include "Application/ShaderSelection.h"
#include "Core/TargetShaderMatcher.h"

namespace NaNBuster
{
	class ShaderPatchService
	{
	public:
		ShaderPatchService(const TargetShaderMatcher& matcher, const IShaderPayload& payload) noexcept;
		ShaderPatchService(const IShaderPatchRegistry& registry, FixPolicy policy) noexcept;

		auto Select(ByteView originalBytes) const noexcept -> ShaderSelection;

	private:
		const TargetShaderMatcher* m_matcher;
		const IShaderPayload* m_payload;
		const IShaderPatchRegistry* m_registry;
		FixPolicy m_policy;
	};
}
