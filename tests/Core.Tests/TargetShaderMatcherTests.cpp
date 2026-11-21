#include "Core/ShaderIdentity.h"
#include "Core/TargetShaderMatcher.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_CORRECTED_SHADER_PATH
#	error "NANBUSTER_CORRECTED_SHADER_PATH must be defined by the test target (absolute path to the generated corrected DXBC)."
#endif

namespace
{
	// Test-side knowledge of the DXBC header layout and the approved target
	// values, transcribed independently from the plan documents.
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;
	constexpr std::size_t DxbcEmbeddedHashSize = 16;
	constexpr std::uint32_t ExpectedBytecodeSize = 1612u;
	constexpr std::uint32_t ExpectedCorrectedSize = 1652u;
	constexpr std::array<std::byte, DxbcEmbeddedHashSize> ExpectedEmbeddedHash = {
		std::byte{ 0x59 }, std::byte{ 0xe1 }, std::byte{ 0xf7 }, std::byte{ 0x82 },
		std::byte{ 0xa4 }, std::byte{ 0xb7 }, std::byte{ 0xb6 }, std::byte{ 0x8d },
		std::byte{ 0xf3 }, std::byte{ 0xff }, std::byte{ 0x42 }, std::byte{ 0xb7 },
		std::byte{ 0xe9 }, std::byte{ 0x24 }, std::byte{ 0x13 }, std::byte{ 0xb1 },
	};

	static_assert(NaNBuster::OriginalTargetIdentity.BytecodeSize == ExpectedBytecodeSize);
	static_assert(NaNBuster::OriginalTargetIdentity.EmbeddedHash == ExpectedEmbeddedHash);
	static_assert(NaNBuster::OriginalTargetIdentity.EmbeddedHash.size() == DxbcEmbeddedHashSize);

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
	const NaNBuster::TargetShaderMatcher matcher;

	// Fixture-owned storage; every view below stays within its lifetime.
	auto original = NaNBuster::TestFixtures::LoadOriginalShader();
	const NaNBuster::ByteView originalView = NaNBuster::TestFixtures::AsByteView(original);
	CHECK(originalView.size() == ExpectedBytecodeSize);

	auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	const NaNBuster::ByteView correctedView = NaNBuster::TestFixtures::AsByteView(corrected);
	CHECK(correctedView.size() == ExpectedCorrectedSize);

	// Exact original accepted.
	CHECK(matcher.Matches(originalView) == true);

	// Corrected oracle (different size, different embedded hash) rejected.
	CHECK(matcher.Matches(correctedView) == false);

	// Empty and truncated views rejected before any hash-byte access.
	CHECK(matcher.Matches(NaNBuster::ByteView{}) == false);
	CHECK(matcher.Matches(NaNBuster::ByteView{ original.data(), 4 }) == false);
	CHECK(matcher.Matches(NaNBuster::ByteView{ original.data(), 19 }) == false);
	CHECK(matcher.Matches(NaNBuster::ByteView{ original.data(), ExpectedBytecodeSize - 1 }) == false);

	// Same prefix plus one extra byte rejected (size checked first).
	std::vector<std::byte> oversized(original);
	oversized.push_back(std::byte{ 0x00 });
	CHECK(oversized.size() == ExpectedBytecodeSize + 1);
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(oversized)) == false);

	// Same-size all-zero buffer rejected (zero hash).
	const std::vector<std::byte> zeroed(ExpectedBytecodeSize, std::byte{ 0x00 });
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(zeroed)) == false);

	// Same-size original with the first/last embedded-hash byte flipped rejected.
	std::vector<std::byte> flipFirst(original);
	flipFirst[DxbcEmbeddedHashOffset] ^= std::byte{ 0xFF };
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(flipFirst)) == false);

	std::vector<std::byte> flipLast(original);
	flipLast[DxbcEmbeddedHashOffset + DxbcEmbeddedHashSize - 1] ^= std::byte{ 0xFF };
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(flipLast)) == false);

	// Arbitrary same-size third-party buffer rejected; its embedded-hash
	// field differs from the target by construction (precondition asserted).
	const std::vector<std::byte> thirdParty(ExpectedBytecodeSize, std::byte{ 0xAA });
	CHECK(!std::equal(
		NaNBuster::OriginalTargetIdentity.EmbeddedHash.begin(),
		NaNBuster::OriginalTargetIdentity.EmbeddedHash.end(),
		thirdParty.data() + DxbcEmbeddedHashOffset));
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(thirdParty)) == false);

	// Repeated calls are deterministic.
	CHECK(matcher.Matches(originalView) == true);
	CHECK(matcher.Matches(originalView) == true);
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(flipFirst)) == false);
	CHECK(matcher.Matches(NaNBuster::TestFixtures::AsByteView(flipFirst)) == false);

	if (g_failures != 0) {
		std::cerr << "Core.TargetShaderMatcher: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.TargetShaderMatcher: all checks passed (original "
			  << originalView.size() << " bytes, corrected " << correctedView.size() << " bytes).\n";
	return EXIT_SUCCESS;
}
