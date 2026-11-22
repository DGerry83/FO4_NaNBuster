#pragma once

#include "Infrastructure/ShaderCreationHook.h"

namespace NaNBuster
{
	class PluginLogger
	{
	public:
		static void LogSkeletonLoaded() noexcept;

		// C12 approved outcomes (INTEGRATION_CONTRACT locked strings, exact).
		static void LogPatchInstalled() noexcept;
		static void LogTargetSubstituted() noexcept;
		static void LogHookUnavailable() noexcept;
		static void LogCorrectedCreationFailedOriginal() noexcept;
	};
}
