#include "Core/LambertShaderIdentities.h"
#include "Core/ShaderIdentityMatcher.h"
#include "TestShaderFixtures.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#ifndef NANBUSTER_LAMBERT_REFERENCE_ROOT
#	error "NANBUSTER_LAMBERT_REFERENCE_ROOT must point to the read-only Lambert reference root."
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
	static_assert(NaNBuster::LambertShaderIdentities.size() == 11);
	static_assert(NaNBuster::LambertShaderIdentities[0].Stage == NaNBuster::LambertShaderStage::Pixel);
	static_assert(NaNBuster::LambertShaderIdentities[10].Stage == NaNBuster::LambertShaderStage::Compute);

	const std::filesystem::path root = NANBUSTER_LAMBERT_REFERENCE_ROOT;
	std::size_t computeCount = 0;
	for (std::size_t index = 0; index < NaNBuster::LambertShaderIdentities.size(); ++index) {
		const auto& entry = NaNBuster::LambertShaderIdentities[index];
		const auto& identity = entry.Identity;
		CHECK(identity.ShaderUid == entry.ReferenceStem);
		CHECK(identity.ShaderUid.size() == 35);
		CHECK(identity.GfxBoosterHash == 0);
		CHECK(identity.AssemblyHash == 0);
		CHECK(entry.Stage == NaNBuster::LambertShaderStage::Pixel || entry.Stage == NaNBuster::LambertShaderStage::Compute);
		computeCount += entry.Stage == NaNBuster::LambertShaderStage::Compute ? 1 : 0;

		for (std::size_t other = index + 1; other < NaNBuster::LambertShaderIdentities.size(); ++other) {
			CHECK(identity.ShaderUid != NaNBuster::LambertShaderIdentities[other].Identity.ShaderUid);
			CHECK(identity.EmbeddedHash != NaNBuster::LambertShaderIdentities[other].Identity.EmbeddedHash);
		}

		const std::filesystem::path dxbcPath = root / entry.ReferenceStem / "original.dxbc";
		const std::filesystem::path asmPath = root / entry.ReferenceStem / "original.asm";
		auto original = NaNBuster::TestFixtures::LoadBinaryFile(dxbcPath.string());
		CHECK(original.size() == identity.BytecodeSize);
		CHECK(std::filesystem::file_size(asmPath) > 0);
		CHECK(NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(original), identity));

		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{}, identity));
		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{ original.data(), 19 }, identity));
		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::ByteView{ original.data(), original.size() - 1 }, identity));

		std::vector<std::byte> oversized(original);
		oversized.push_back(std::byte{ 0x00 });
		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(oversized), identity));

		std::vector<std::byte> flipped(original);
		flipped[4] ^= std::byte{ 0xFF };
		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(flipped), identity));
		flipped = original;
		flipped[19] ^= std::byte{ 0xFF };
		CHECK(!NaNBuster::ShaderIdentityMatcher::Matches(NaNBuster::TestFixtures::AsByteView(flipped), identity));
	}
	CHECK(computeCount == 1);

	const auto invalidHash = NaNBuster::LambertIdentityDetail::EmbeddedHashFromText("invalid");
	CHECK((invalidHash == std::array<std::byte, 16>{}));

	if (g_failures != 0) {
		std::cerr << "Core.LambertShaderIdentity: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.LambertShaderIdentity: all checks passed (11 source identities, 10 pixel, 1 compute).\n";
	return EXIT_SUCCESS;
}
