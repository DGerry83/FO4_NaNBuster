#pragma once

#include "Application/ShaderPatchService.h"
#include "Core/TargetShaderMatcher.h"
#include "Infrastructure/EmbeddedShaderPayload.h"
#include "Infrastructure/EngineShaderCreator.h"
#include "Infrastructure/ShaderCreationHook.h"

namespace NaNBuster
{
	// C13 process-lifetime owner of the match/substitute pipeline
	// (INTEGRATION_CONTRACT "Process-lifetime composition"). PluginEntry's
	// static instance is the sole owner; there is no service locator and no
	// mutable domain global. The C12 replay callback and the production outcome
	// logger are constructor-injected. Member declaration order guarantees the
	// matcher and payload outlive the service, and the service and creator
	// outlive the hook.
	class Composition
	{
	public:
		Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger) noexcept;

		// Installs the C12 hook exactly once (the hook itself latches attempts).
		// Returns the feature result: false means the hook is a safely disabled
		// feature (HookUnavailable logged once by the hook); the host load does
		// not fail. Non-const so it can drive the stateful Install.
		[[nodiscard]] auto Initialize() noexcept -> bool;

	private:
		TargetShaderMatcher m_matcher;
		EmbeddedShaderPayload m_payload;
		ShaderPatchService m_service;
		EngineShaderCreator m_creator;
		ShaderCreationHook m_hook;
	};
}
