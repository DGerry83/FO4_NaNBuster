#include "Core/ShaderIdentity.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

namespace
{
	// Test-side knowledge of the DXBC header layout (container parsing is C3 scope).
	constexpr std::size_t DxbcMagicOffset = 0;
	constexpr std::size_t DxbcMagicSize = 4;
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;

	// Independent expected values transcribed from the approved plan documents.
	constexpr std::string_view ExpectedShaderUid = "PS49B6A21CI0O1";
	constexpr std::uint32_t ExpectedBytecodeSize = 1612u;
	constexpr std::array<std::byte, 16> ExpectedEmbeddedHash = {
		std::byte{ 0x59 }, std::byte{ 0xe1 }, std::byte{ 0xf7 }, std::byte{ 0x82 },
		std::byte{ 0xa4 }, std::byte{ 0xb7 }, std::byte{ 0xb6 }, std::byte{ 0x8d },
		std::byte{ 0xf3 }, std::byte{ 0xff }, std::byte{ 0x42 }, std::byte{ 0xb7 },
		std::byte{ 0xe9 }, std::byte{ 0x24 }, std::byte{ 0x13 }, std::byte{ 0xb1 },
	};
	constexpr std::uint32_t ExpectedGfxBoosterHash = 0xF823EE3Fu;
	constexpr std::uint32_t ExpectedAssemblyHash = 0x49B6A21Cu;
	constexpr std::array<char, DxbcMagicSize> ExpectedMagic = { 'D', 'X', 'B', 'C' };

	static_assert(NaNBuster::OriginalTargetIdentity.ShaderUid == ExpectedShaderUid);
	static_assert(NaNBuster::OriginalTargetIdentity.BytecodeSize == ExpectedBytecodeSize);
	static_assert(NaNBuster::OriginalTargetIdentity.EmbeddedHash == ExpectedEmbeddedHash);
	static_assert(NaNBuster::OriginalTargetIdentity.GfxBoosterHash == ExpectedGfxBoosterHash);
	static_assert(NaNBuster::OriginalTargetIdentity.AssemblyHash == ExpectedAssemblyHash);

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
	// Identity constants (runtime re-verification; compile-time via static_assert above).
	CHECK(NaNBuster::OriginalTargetIdentity.ShaderUid == ExpectedShaderUid);
	CHECK(NaNBuster::OriginalTargetIdentity.BytecodeSize == ExpectedBytecodeSize);
	CHECK(NaNBuster::OriginalTargetIdentity.EmbeddedHash == ExpectedEmbeddedHash);
	CHECK(NaNBuster::OriginalTargetIdentity.GfxBoosterHash == ExpectedGfxBoosterHash);
	CHECK(NaNBuster::OriginalTargetIdentity.AssemblyHash == ExpectedAssemblyHash);

	// Original reference file: read-only load into fixture-owned storage.
	auto original = NaNBuster::TestFixtures::LoadOriginalShader();
	const NaNBuster::ByteView originalView = NaNBuster::TestFixtures::AsByteView(original);

	CHECK(originalView.size() == NaNBuster::OriginalTargetIdentity.BytecodeSize);
	CHECK(originalView.size() >= DxbcEmbeddedHashOffset + NaNBuster::OriginalTargetIdentity.EmbeddedHash.size());

	if (originalView.size() >= DxbcMagicSize) {
		CHECK(std::memcmp(originalView.data() + DxbcMagicOffset, ExpectedMagic.data(), ExpectedMagic.size()) == 0);
	}

	if (originalView.size() >= DxbcEmbeddedHashOffset + NaNBuster::OriginalTargetIdentity.EmbeddedHash.size()) {
		const auto* embedded = originalView.data() + DxbcEmbeddedHashOffset;
		CHECK(std::equal(
			NaNBuster::OriginalTargetIdentity.EmbeddedHash.begin(),
			NaNBuster::OriginalTargetIdentity.EmbeddedHash.end(),
			embedded));
		CHECK(std::equal(
			ExpectedEmbeddedHash.begin(),
			ExpectedEmbeddedHash.end(),
			embedded));
	}

	if (g_failures != 0) {
		std::cerr << "Core.ShaderIdentity: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.ShaderIdentity: all checks passed (" << originalView.size() << " original bytes).\n";
	return EXIT_SUCCESS;
}
