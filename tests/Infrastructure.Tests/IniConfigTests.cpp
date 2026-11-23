#include "Infrastructure/IniConfig.h"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef NANBUSTER_DEFAULT_INI_PATH
#	error "NANBUSTER_DEFAULT_INI_PATH is required."
#endif

namespace
{
	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

	void WriteFile(const std::filesystem::path& path, const std::string& content)
	{
		std::ofstream stream(path, std::ios::trunc);
		stream << content;
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

auto main() -> int
{
	const auto unique = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	const auto root = std::filesystem::temp_directory_path() / ("nanbuster-ini-" + std::to_string(unique));
	std::filesystem::create_directories(root);
	const auto path = root / "NaNBuster.ini";

	{
		const auto result = NaNBuster::IniConfigLoader::Load(root / "missing.ini");
		CHECK(result.Policy.Color0LogClampEnabled);
		CHECK(result.Policy.LambertDecodeGuardEnabled);
		CHECK(!result.PathError);
		CHECK(!result.Color0LogClampInvalid);
		CHECK(!result.LambertDecodeGuardInvalid);
	}
	{
		const auto result = NaNBuster::IniConfigLoader::Load({});
		CHECK(result.PathError);
		CHECK(result.Policy.Color0LogClampEnabled);
		CHECK(result.Policy.LambertDecodeGuardEnabled);
	}

	WriteFile(path, "");
	auto result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(result.Policy.LambertDecodeGuardEnabled);

	WriteFile(path, "[Fixes]\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(result.Policy.LambertDecodeGuardEnabled);

	WriteFile(path, "[Fixes]\nColor0LogClamp=false\nLambertDecodeGuard=false\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(!result.Policy.Color0LogClampEnabled);
	CHECK(!result.Policy.LambertDecodeGuardEnabled);
	CHECK(!result.Color0LogClampInvalid);
	CHECK(!result.LambertDecodeGuardInvalid);

	WriteFile(path, "[Fixes]\nColor0LogClamp=0\nLambertDecodeGuard=1\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(!result.Policy.Color0LogClampEnabled);
	CHECK(result.Policy.LambertDecodeGuardEnabled);

	WriteFile(path, "[Fixes]\nColor0LogClamp=TRUE\nLambertDecodeGuard=FaLsE\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(!result.Policy.LambertDecodeGuardEnabled);

	WriteFile(path, "[Fixes]\nColor0LogClamp=maybe\nLambertDecodeGuard=invalid\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(result.Policy.LambertDecodeGuardEnabled);
	CHECK(result.Color0LogClampInvalid);
	CHECK(result.LambertDecodeGuardInvalid);

	WriteFile(path, "[Fixes]\nColor0LogClamp=\nLambertDecodeGuard=false\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(!result.Policy.LambertDecodeGuardEnabled);
	CHECK(result.Color0LogClampInvalid);
	CHECK(!result.LambertDecodeGuardInvalid);

	WriteFile(path, "[Fixes]\nColor0LogClamp=  true  \nUnknownSetting=false\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(result.Policy.LambertDecodeGuardEnabled);
	CHECK(!result.Color0LogClampInvalid);
	CHECK(!result.LambertDecodeGuardInvalid);

	WriteFile(path, "[Fixes]\nColor0LogClamp=" + std::string(128, 'x') + "\n");
	result = NaNBuster::IniConfigLoader::Load(path);
	CHECK(result.Policy.Color0LogClampEnabled);
	CHECK(result.Color0LogClampInvalid);

	const auto defaultPath = NaNBuster::IniConfigLoader::DefaultPath();
	CHECK(!defaultPath.empty());
	CHECK(defaultPath.filename() == "NaNBuster.ini");

	const auto packaged = NaNBuster::IniConfigLoader::Load(NANBUSTER_DEFAULT_INI_PATH);
	CHECK(packaged.Policy.Color0LogClampEnabled);
	CHECK(packaged.Policy.LambertDecodeGuardEnabled);
	CHECK(!packaged.Color0LogClampInvalid);
	CHECK(!packaged.LambertDecodeGuardInvalid);

	std::filesystem::remove_all(root);
	if (g_failures != 0) {
		std::cerr << "Infrastructure.IniConfig: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.IniConfig: all checks passed (defaults, toggles, invalid values, default path).\n";
	return EXIT_SUCCESS;
}
