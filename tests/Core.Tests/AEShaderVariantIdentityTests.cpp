#include "Core/AEShaderVariantIdentities.h"
#include "Core/ShaderIdentityMatcher.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#ifndef NANBUSTER_AE_REFERENCE_ROOT
#error "NANBUSTER_AE_REFERENCE_ROOT must point to the read-only AE variant reference root."
#endif

namespace
{
int g_failures = 0;

void Check(bool condition, const char *expression, const char *file, int line)
{
	if (!condition)
	{
		++g_failures;
		std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

void CheckOriginal(const NaNBuster::ShaderIdentity &identity, const std::filesystem::path &path)
{
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile(path.string());
	CHECK(original.size() == identity.BytecodeSize);
	CHECK(NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(original), identity));
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{}, identity));
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{original.data(), 19}, identity));
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{original.data(), original.size() - 1},
													 identity));

	std::vector<std::byte> oversized(original);
	oversized.push_back(std::byte{0x00});
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(oversized), identity));

	std::vector<std::byte> flipped(original);
	flipped[4] ^= std::byte{0xFF};
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(flipped), identity));
	flipped = original;
	flipped[19] ^= std::byte{0xFF};
	CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(flipped), identity));
}
} // namespace

auto main() -> int
{
	static_assert(NaNBuster::AELambertShaderIdentities.size() == 12);
	constexpr std::array<std::string_view, 12> expectedUids{
		"42c01806-2fff2213-454d5b47-0febbec5-10000004", "d259278e-5d6465c2-9597f7a5-9167a174-10000204",
		"62305116-5f1dbb95-5bf460a1-66fc3e36-00000600", "a142467d-0ccdc0c4-42599f57-524ba8ef-00008204",
		"8fae50bb-c275fe55-a0b651bd-6b0232cc-00020000", "fb40c999-0825a3b1-0d1db084-782331d8-00080208",
		"9a38981c-14b2c2d0-83512c06-872c2a92-0c400282", "34500494-908064b9-67601b25-87421985-09200202",
		"c249f76f-01471f14-96e7c782-0ab3dea2-00000a20", "190e840a-34210f13-e95f808f-6f3f155a-09220202",
		"471e194d-c15a3b7c-a377e02c-723a6df5-00000000", "2184f4ef-f68e8e50-a5e4109f-7c322184-00000001",
	};
	constexpr std::array<std::uint32_t, 12> expectedSizes{
		4052, 5624, 5916, 5360, 2500, 6868, 2240, 25560, 3048, 26292, 3892, 6444,
	};
	const std::filesystem::path root = NANBUSTER_AE_REFERENCE_ROOT;
	std::size_t computeCount = 0;
	for (std::size_t index = 0; index < NaNBuster::AELambertShaderIdentities.size(); ++index)
	{
		const auto &entry = NaNBuster::AELambertShaderIdentities[index];
		const auto &identity = entry.Identity;
		CHECK(identity.ShaderUid == expectedUids[index]);
		CHECK(identity.ShaderUid.size() == 44);
		CHECK(identity.BytecodeSize == expectedSizes[index]);
		CHECK(identity.GfxBoosterHash == 0);
		CHECK(identity.AssemblyHash == 0);
		CHECK(entry.ReferenceStem == NaNBuster::LambertShaderIdentities[index].ReferenceStem);
		computeCount += entry.Stage == NaNBuster::LambertShaderStage::Compute ? 1 : 0;

		for (std::size_t other = index + 1; other < NaNBuster::AELambertShaderIdentities.size(); ++other)
		{
			CHECK(identity.ShaderUid != NaNBuster::AELambertShaderIdentities[other].Identity.ShaderUid);
			CHECK(identity.EmbeddedHash != NaNBuster::AELambertShaderIdentities[other].Identity.EmbeddedHash);
		}
		CHECK(identity.ShaderUid != NaNBuster::AEIsotropicSpecularTargetIdentity.ShaderUid);
		CHECK(identity.EmbeddedHash != NaNBuster::AEIsotropicSpecularTargetIdentity.EmbeddedHash);
		CheckOriginal(identity, root / entry.ReferenceStem / "original.dxbc");
	}
	CHECK(computeCount == 2);

	const auto &isotropic = NaNBuster::AEIsotropicSpecularTargetIdentity;
	CHECK(isotropic.ShaderUid == "4f02f995-8e33cdb3-ac24ad49-b3a5c778-00080220");
	CHECK(isotropic.ShaderUid.size() == 44);
	CHECK(isotropic.BytecodeSize == 6516);
	CHECK(isotropic.GfxBoosterHash == 0);
	CHECK(isotropic.AssemblyHash == 0);
	CheckOriginal(isotropic, root / NaNBuster::AEIsotropicSpecularReferenceStem / "original.dxbc");

	if (g_failures != 0)
	{
		std::cerr << "Core.AEShaderVariantIdentity: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Core.AEShaderVariantIdentity: all checks passed (13 identities, exact matching, rejection).\n";
	return EXIT_SUCCESS;
}
