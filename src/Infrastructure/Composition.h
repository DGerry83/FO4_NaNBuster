#pragma once

#include "Application/ShaderPatchService.h"
#include "Core/TargetShaderMatcher.h"
#include "Infrastructure/ComputeShaderCreationHook.h"
#include "Infrastructure/EmbeddedShaderPatchRegistry.h"
#include "Infrastructure/EmbeddedShaderPayload.h"
#include "Infrastructure/EngineShaderCreator.h"
#include "Infrastructure/ShaderCreationHook.h"

namespace NaNBuster
{
	class Composition
	{
	public:
		Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger) noexcept;
		Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger, FixPolicy policy) noexcept;
		Composition(
			OriginalCreationCallback replay,
			OriginalCreationCallback computeReplay,
			OutcomeLogger outcomeLogger,
			FixPolicy policy,
			SubstitutionLogger substitutionLogger = nullptr) noexcept;

		[[nodiscard]] auto Initialize() noexcept -> bool;

	private:
		TargetShaderMatcher m_matcher;
		EmbeddedShaderPayload m_payload;
		EmbeddedShaderPatchRegistry m_registry;
		ShaderPatchService m_service;
		EngineShaderCreator m_creator;
		ShaderCreationHook m_hook;
		EngineShaderCreator m_computeCreator;
		ComputeShaderCreationHook m_computeHook;
	};
}
