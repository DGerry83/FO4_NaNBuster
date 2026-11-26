#include "Core/LambertShaderIdentities.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
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
#ifndef NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR
#	error "NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR is required."
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

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

	auto ExpectedBytes(std::size_t index, const std::filesystem::path& outputRoot) -> std::vector<std::byte>
	{
		if (index == 0) {
			return NaNBuster::TestFixtures::LoadCorrectedShader();
		}
		if (index == 27) {
			return NaNBuster::TestFixtures::LoadBinaryFile(
				(std::filesystem::path(NANBUSTER_ISOTROPIC_SHADER_OUTPUT_DIR) / "patched.dxbc").string());
		}
		const auto targetRoot = outputRoot / NaNBuster::LambertShaderIdentities[1].ReferenceStem;
		if (index == 1) {
			return NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "combined.dxbc").string());
		}
		if (index == 2) {
			return NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / "anisotropic-clamped.dxbc").string());
		}
		const auto computeRoot = outputRoot / NaNBuster::LambertShaderIdentities[11].ReferenceStem;
		if (index == 3) {
			return NaNBuster::TestFixtures::LoadBinaryFile((computeRoot / "combined.dxbc").string());
		}
		if (index == 4) {
			return NaNBuster::TestFixtures::LoadBinaryFile((computeRoot / "anisotropic-clamped.dxbc").string());
		}
		if (index >= 5 && index <= 14) {
			const std::size_t targetIndices[] = { 2, 3, 5, 7, 9 };
			const auto targetRoot = outputRoot / NaNBuster::LambertShaderIdentities[targetIndices[(index - 5) / 2]].ReferenceStem;
			return NaNBuster::TestFixtures::LoadBinaryFile(
				(targetRoot / (index % 2 == 1 ? "combined.dxbc" : "anisotropic-clamped.dxbc")).string());
		}
		return NaNBuster::TestFixtures::LoadBinaryFile(
			(outputRoot / NaNBuster::LambertShaderIdentities[index - 15].ReferenceStem / "patched.dxbc").string());
	}
}

auto main() -> int
{
	const NaNBuster::EmbeddedShaderPatchRegistry registry;
	const auto entries = registry.Entries();
	CHECK(entries.size() == 28);

	const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
	for (std::size_t i = 0; i < entries.size(); ++i) {
		const auto& entry = entries[i];
		const auto expectedBytes = ExpectedBytes(i, outputRoot);
		CHECK(entry.CorrectedBytecode.size() == expectedBytes.size());
		CHECK(std::equal(entry.CorrectedBytecode.begin(), entry.CorrectedBytecode.end(), expectedBytes.begin()));

		if (i == 0) {
			CHECK(entry.Fix == NaNBuster::FixId::Color0LogClamp);
			CHECK(entry.SourceIdentity == &NaNBuster::OriginalTargetIdentity);
		} else if (i == 1) {
			CHECK(entry.Fix == NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[1].Identity);
		} else if (i == 2) {
			CHECK(entry.Fix == NaNBuster::FixId::AnisotropicSpecularClamp);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[1].Identity);
		} else if (i == 3) {
			CHECK(entry.Fix == NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[11].Identity);
		} else if (i == 4) {
			CHECK(entry.Fix == NaNBuster::FixId::AnisotropicSpecularClamp);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[11].Identity);
		} else if (i >= 5 && i <= 14) {
			const std::size_t targetIndices[] = { 2, 3, 5, 7, 9 };
			CHECK(entry.Fix == (i % 2 == 1 ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp : NaNBuster::FixId::AnisotropicSpecularClamp));
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[targetIndices[(i - 5) / 2]].Identity);
		} else if (i == 27) {
			CHECK(entry.Fix == NaNBuster::FixId::IsotropicSpecularAA);
			CHECK(entry.SourceIdentity == &NaNBuster::IsotropicSpecularTargetIdentity);
		} else {
			CHECK(entry.Fix == NaNBuster::FixId::LambertDecodeGuard);
			CHECK(entry.SourceIdentity == &NaNBuster::LambertShaderIdentities[i - 15].Identity);
		}
	}

	CHECK(registry.Entries().data() == entries.data());
	const NaNBuster::EmbeddedShaderPatchRegistry second;
	CHECK(second.Entries().data() == entries.data());

	constexpr NaNBuster::FixPolicy defaults;
	CHECK(defaults.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(defaults.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	CHECK(defaults.IsEnabled(NaNBuster::FixId::AnisotropicSpecularClamp));
	CHECK(defaults.IsEnabled(NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp));
	CHECK(defaults.IsEnabled(NaNBuster::FixId::IsotropicSpecularAA));
	constexpr NaNBuster::FixPolicy colorOff{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = true };
	CHECK(!colorOff.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(colorOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	constexpr NaNBuster::FixPolicy lambertOff{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = false };
	CHECK(lambertOff.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(!lambertOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	CHECK(lambertOff.IsEnabled(NaNBuster::FixId::AnisotropicSpecularClamp));
	CHECK(!lambertOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp));
	constexpr NaNBuster::FixPolicy anisotropicOff{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = true, .AnisotropicSpecularClampEnabled = false };
	CHECK(!anisotropicOff.IsEnabled(NaNBuster::FixId::AnisotropicSpecularClamp));
	CHECK(!anisotropicOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp));
	CHECK(anisotropicOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	constexpr NaNBuster::FixPolicy bothOff{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = false, .AnisotropicSpecularClampEnabled = false };
	CHECK(!bothOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp));
	constexpr NaNBuster::FixPolicy isotropicOff{ .IsotropicSpecularAAEnabled = false };
	CHECK(!isotropicOff.IsEnabled(NaNBuster::FixId::IsotropicSpecularAA));
	CHECK(isotropicOff.IsEnabled(NaNBuster::FixId::Color0LogClamp));
	CHECK(isotropicOff.IsEnabled(NaNBuster::FixId::LambertDecodeGuard));
	CHECK(isotropicOff.IsEnabled(NaNBuster::FixId::AnisotropicSpecularClamp));

	NaNBuster::ByteView surviving;
	{
		const NaNBuster::EmbeddedShaderPatchRegistry scoped;
		surviving = scoped.Entries()[15].CorrectedBytecode;
	}
	const auto expected = ExpectedBytes(15, outputRoot);
	CHECK(std::equal(surviving.begin(), surviving.end(), expected.begin()));

	if (g_failures != 0) {
		std::cerr << "Infrastructure.EmbeddedShaderPatchRegistry: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.EmbeddedShaderPatchRegistry: all checks passed (28 immutable entries, stable views, policy tags).\n";
	return EXIT_SUCCESS;
}
