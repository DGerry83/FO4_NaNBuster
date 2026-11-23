#include "AppConfig.h"
#include "Infrastructure/Composition.h"
#include "Infrastructure/IniConfig.h"
#include "Infrastructure/PluginLogger.h"

// The pinned CommonLibF4 AV resolves F4SEPlugin_Version internally during
// F4SE::Init; OG F4SE itself ignores it and uses Query/Load. The generated
// XsePluginInfo.cpp emits this symbol only for NG/AE, so the OG-only
// configuration must define exactly one instance here. The guard prevents a
// duplicate definition if NG/AE are enabled later.
#if COMMONLIB_RUNTIME_OG == 1 && COMMONLIB_RUNTIME_NG == 0 && COMMONLIB_RUNTIME_AE == 0
F4SE_PLUGIN_VERSION = []() consteval noexcept {
	auto versionData = F4SE::PluginVersionData();

	versionData.SetPluginName(NANBUSTER_PLUGIN_NAME);
	versionData.SetPluginAuthor(NANBUSTER_PLUGIN_AUTHOR);
	versionData.SetPluginVersion(REX::Version(NANBUSTER_VERSION_MAJOR, NANBUSTER_VERSION_MINOR, NANBUSTER_VERSION_PATCH, NANBUSTER_VERSION_TWEAK));

	versionData.SetUseSignatureScanning(false);
	versionData.SetUseNoStructs(false);
	versionData.SetCompatibleVersions({ F4SE::RUNTIME_LATEST_OG });

	return versionData;
}();
#endif

namespace
{
	bool InitializePlugin(const F4SE::LoadInterface* a_f4se)
	{
		static std::once_flag once;
		static bool initialized = false;
		std::call_once(once, [&]() {
			F4SE::Init(a_f4se);
			const auto config = NaNBuster::IniConfigLoader::Load(NaNBuster::IniConfigLoader::DefaultPath());
			NaNBuster::PluginLogger::LogConfiguration(config);
			// C13: process-lifetime composition with the injected C12 replay
			// callback and the production outcome logger. The hook feature
			// result is discarded: an unavailable hook is a safely disabled
			// feature (one approved diagnostic), never a host-load failure.
			static NaNBuster::Composition composition(
				NaNBuster::HookReplayCreationCallback,
				NaNBuster::ComputeShaderReplayCreationCallback,
				NaNBuster::PluginLoggerOutcomeLogger(),
				config.Policy,
				NaNBuster::PluginLoggerSubstitutionLogger());
			static_cast<void>(composition.Initialize());
			initialized = true;
		});
		return initialized;
	}

	F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
	{
		return InitializePlugin(a_f4se);
	}

	F4SE_PLUGIN_PRELOAD(const F4SE::LoadInterface* a_f4se)
	{
		return InitializePlugin(a_f4se);
	}
}
