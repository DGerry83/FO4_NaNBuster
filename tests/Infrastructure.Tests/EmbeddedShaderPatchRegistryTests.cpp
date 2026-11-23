#include "Core/LambertShaderIdentities.h"
#include "Infrastructure/EmbeddedShaderPatchRegistry.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_CORRECTED_SHADER_PATH
#	error "NANBUSTER_CORRECTED_SHADER_PATH is required."
#endif
#ifndef NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT
#	error "NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT is required."
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
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

auto main() -> int
{
	const NaNBuster::EmbeddedShaderPatchRegistry registry;
	const auto entries = registry.Entries();
	CHECK(entries.size() == 12);
	CHECK(entries[0].Fix == NaNBuster::FixId::Color0LogClamp);
	CHECK(entries[0].SourceIdentity == &NaNBuster::OriginalTargetIdentity);

	const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
	for (std::size_t i = 0; i < entries.size(); ++i) {
		const auto& entry = entries[i];
		const auto expectedBytes = i == 0
			? NaNBuster::TestFixtures::LoadCorrectedShader()
			: NaNBuster::TestFixtures::LoadBinaryFile(
				(outputRoot / NaNBuster::LambertShaderIdentities[i - 1].ReferenceStem / "patched.dxbc").string());
		CHECK(entry.CorrectedBytecode.size() == expectedBytes.size());
		CHECK(std::equal(entry.CorrectedBytecode.begin(), entry.CorrectedBytecode.end(), expectedBytes.begin()));

		if (i == 0) {
			CHECK(entry.Fix == NaNBuster::FixId::Color0LogClamp);
		} else {
			CHECK(entry.Fix == NaNBuster::FixId::LambertDecodeGuard);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[i - 1].Identity);
		}
		for (std::size_t other = i + 1; other < entries.size(); ++other) {
			CHECK(entry.SourceIdentity != entries[other].SourceIdentity);
		}
	}

	CHECK(registry.Entries().data() == entries.data());
	const NaNBuster::EmbeddedShaderPatchRegistry second;
	CHECK(second.Entries().data() == entries.data());

	constexpr NaNBuster::FixPolicy defaults;
	CHECK(defaults.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(defaults.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	constexpr NaNBuster::FixPolicy colorOff{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = true };
	CHECK(!colorOff.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(colorOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	constexpr NaNBuster::FixPolicy lambertOff{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = false };
	CHECK(lambertOff.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(!lambertOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));

	NaNBuster::ByteView surviving;
	{
		const NaNBuster::EmbeddedShaderPatchRegistry scoped;
		surviving = scoped.Entries()[1].CorrectedBytecode;
	}
	const auto expected = NaNBuster::TestFixtures::LoadBinaryFile(
		(outputRoot / NaNBuster::LambertShaderIdentities[0].ReferenceStem / "patched.dxbc").string());
	CHECK(std::equal(surviving.begin(), surviving.end(), expected.begin()));

	if (g_failures != 0) {
		std::cerr << "Infrastructure.EmbeddedShaderPatchRegistry: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.EmbeddedShaderPatchRegistry: all checks passed (12 immutable entries, stable views).\n";
	return EXIT_SUCCESS;
}
