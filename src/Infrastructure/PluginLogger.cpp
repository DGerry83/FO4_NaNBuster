#include "Infrastructure/PluginLogger.h"

#include "AppConfig.h"

namespace NaNBuster
{
	void PluginLogger::LogSkeletonLoaded() noexcept
	{
		REX::LogInformation("{} v{} skeleton loaded; shader hook is not installed."sv, PluginName, PluginVersion);
	}
}
