#include "Infrastructure/PluginLogger.h"

#include "AppConfig.h"

namespace NaNBuster
{
	void PluginLogger::LogSkeletonLoaded() noexcept
	{
		REX::LogInformation("{} v{} skeleton loaded; shader hook is not installed."sv, PluginName, PluginVersion);
	}

	void PluginLogger::LogConfiguration(const IniConfigLoadResult& config) noexcept
	{
		if (config.PathError) {
			REX::LogInformation("NaNBuster: configuration path unavailable; using enabled defaults for all fixes.");
		}
		if (config.Color0LogClampInvalid) {
			REX::LogInformation("NaNBuster: invalid Color0LogClamp value; using enabled default.");
		}
		if (config.LambertDecodeGuardInvalid) {
			REX::LogInformation("NaNBuster: invalid LambertDecodeGuard value; using enabled default.");
		}
		REX::LogInformation(
			"NaNBuster: Color0LogClamp={}; LambertDecodeGuard={}."sv,
			config.Policy.Color0LogClampEnabled ? "enabled" : "disabled",
			config.Policy.LambertDecodeGuardEnabled ? "enabled" : "disabled");
	}

	void PluginLogger::LogPatchInstalled() noexcept
	{
		REX::LogInformation("NaNBuster: shader NaN patch installed for PS49B6A21CI0O1.");
	}

	void PluginLogger::LogTargetSubstituted() noexcept
	{
		REX::LogInformation("NaNBuster: patched PS49B6A21CI0O1 (82f7e159-8db6b7a4-b742fff3-b11324e9).");
	}

	void PluginLogger::LogHookUnavailable() noexcept
	{
		REX::LogInformation("NaNBuster: shader hook unavailable; patch disabled.");
	}

	void PluginLogger::LogCorrectedCreationFailedOriginal() noexcept
	{
		REX::LogInformation("NaNBuster: patched shader creation failed; using original shader.");
	}

	auto PluginLoggerSubstitutionLogger() noexcept -> SubstitutionLogger
	{
		return +[](const ShaderIdentity& identity) noexcept {
			REX::LogInformation("NaNBuster: patched {}."sv, identity.ShaderUid);
		};
	}

	auto PluginLoggerOutcomeLogger() noexcept -> OutcomeLogger
	{
		return +[](HookOutcome outcome) noexcept {
			switch (outcome) {
			case HookOutcome::PatchInstalled:
				PluginLogger::LogPatchInstalled();
				break;
			case HookOutcome::TargetSubstituted:
				PluginLogger::LogTargetSubstituted();
				break;
			case HookOutcome::HookUnavailable:
				PluginLogger::LogHookUnavailable();
				break;
			case HookOutcome::CorrectedCreationFailedOriginal:
				PluginLogger::LogCorrectedCreationFailedOriginal();
				break;
			}
		};
	}
}
