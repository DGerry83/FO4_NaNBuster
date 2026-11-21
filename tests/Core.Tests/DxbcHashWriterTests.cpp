#include "Core/DxbcClampPatcher.h"
#include "Core/DxbcContainer.h"
#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/ShaderIdentity.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
	using NaNBuster::ByteView;
	using NaNBuster::DxbcErrorCode;
	using NaNBuster::DxbcInspector;

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

namespace
{
	using Bytes = std::vector<std::byte>;

	void PutU32(Bytes& bytes, std::uint32_t value)
	{
		for (unsigned shift = 0; shift < 32; shift += 8) {
			bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFu));
		}
	}

	std::uint32_t GetU32(const Bytes& bytes, std::size_t offset)
	{
		std::uint32_t value = 0;
		for (unsigned i = 0; i < 4; ++i) {
			value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
		}
		return value;
	}

	// ---- Synthetic minimal container builder (same layout as the C3 test) ----

	auto MakeMinimalContainer(std::size_t retCount) -> Bytes
	{
		constexpr std::uint32_t kRet = 0x0100003Eu; // ret (opcode 62, len 1)

		Bytes shexContent;
		PutU32(shexContent, 0x50u); // ps_5_0
		PutU32(shexContent, static_cast<std::uint32_t>(retCount) + 2u);
		for (std::size_t i = 0; i < retCount; ++i) {
			PutU32(shexContent, kRet);
		}

		Bytes bytes;
		bytes.insert(bytes.end(), { std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ 'B' }, std::byte{ 'C' } });
		for (int i = 0; i < 16; ++i) {
			bytes.push_back(std::byte{ 0 }); // hash field, filled by Write
		}
		PutU32(bytes, 1u); // version
		PutU32(bytes, static_cast<std::uint32_t>(44 + shexContent.size())); // total
		PutU32(bytes, 1u); // chunk count
		PutU32(bytes, 36u); // chunk table: SHEX at 32 + 4
		bytes.insert(bytes.end(), { std::byte{ 'S' }, std::byte{ 'H' }, std::byte{ 'E' }, std::byte{ 'X' } });
		PutU32(bytes, static_cast<std::uint32_t>(shexContent.size()));
		bytes.insert(bytes.end(), shexContent.begin(), shexContent.end());
		return bytes;
	}

	auto HashField(const Bytes& bytes) -> std::array<std::byte, 16>
	{
		std::array<std::byte, 16> hash{};
		for (std::size_t i = 0; i < hash.size(); ++i) {
			hash[i] = bytes[4 + i];
		}
		return hash;
	}

	void CheckHashField(const Bytes& bytes, const std::array<std::byte, 16>& expected, const char* file, int line)
	{
		const auto actual = HashField(bytes);
		Check(actual == expected, "embedded hash field", file, line);
		if (actual != expected) {
			std::cerr << "  expected ";
			for (const auto b : expected) {
				std::cerr << std::hex << static_cast<unsigned>(b) << ' ';
			}
			std::cerr << "\n  actual   ";
			for (const auto b : actual) {
				std::cerr << std::hex << static_cast<unsigned>(b) << ' ';
			}
			std::cerr << std::dec << '\n';
		}
	}

#define CHECK_HASH_FIELD(bytes, expected) CheckHashField((bytes), (expected), __FILE__, __LINE__)

	// Every byte outside [4, 20) is identical between a and b.
	void CheckNonHashBytesUnchanged(const Bytes& a, const Bytes& b, const char* file, int line)
	{
		Check(a.size() == b.size(), "size preserved", file, line);
		bool unchanged = a.size() == b.size();
		if (unchanged) {
			for (std::size_t i = 0; i < a.size(); ++i) {
				if (i >= 4 && i < 20) {
					continue;
				}
				if (a[i] != b[i]) {
					std::cerr << "  non-hash byte changed at 0x" << std::hex << i << std::dec << '\n';
					unchanged = false;
					break;
				}
			}
		}
		Check(unchanged, "non-hash bytes unchanged", file, line);
	}

#define CHECK_NONHASH_UNCHANGED(a, b) CheckNonHashBytesUnchanged((a), (b), __FILE__, __LINE__)

	// ---- Original oracle tests ----

	void OriginalRestoreTests()
	{
		auto original = NaNBuster::TestFixtures::LoadOriginalShader();
		CHECK(original.size() == 1612u);

		const NaNBuster::DxbcHashWriter writer;
		const auto& expectedHash = NaNBuster::OriginalTargetIdentity.EmbeddedHash;
		const std::array<std::byte, 16> scrambled = {
			std::byte{ 0xA5 }, std::byte{ 0x5A }, std::byte{ 0xFF }, std::byte{ 0x00 },
			std::byte{ 0x11 }, std::byte{ 0x22 }, std::byte{ 0x33 }, std::byte{ 0x44 },
			std::byte{ 0x55 }, std::byte{ 0x66 }, std::byte{ 0x77 }, std::byte{ 0x88 },
			std::byte{ 0x99 }, std::byte{ 0xAA }, std::byte{ 0xBB }, std::byte{ 0xCC },
		};

		{
			Bytes zeroed = original;
			for (std::size_t i = 4; i < 20; ++i) {
				zeroed[i] = std::byte{ 0 };
			}
			const Bytes before = zeroed;
			auto result = writer.Write(std::span<std::byte>{ zeroed.data(), zeroed.size() });
			CHECK(result.has_value());
			CHECK(zeroed == original);
			CHECK_HASH_FIELD(zeroed, expectedHash);
			CHECK_NONHASH_UNCHANGED(before, zeroed);
		}
		{
			Bytes scrambledCopy = original;
			for (std::size_t i = 0; i < 16; ++i) {
				scrambledCopy[4 + i] = scrambled[i];
			}
			const Bytes before = scrambledCopy;
			auto result = writer.Write(std::span<std::byte>{ scrambledCopy.data(), scrambledCopy.size() });
			CHECK(result.has_value());
			CHECK(scrambledCopy == original);
			CHECK_HASH_FIELD(scrambledCopy, expectedHash);
			CHECK_NONHASH_UNCHANGED(before, scrambledCopy);
		}
		{
			Bytes untouched = original;
			const Bytes before = untouched;
			auto result = writer.Write(std::span<std::byte>{ untouched.data(), untouched.size() });
			CHECK(result.has_value());
			CHECK(untouched == original);
			CHECK_HASH_FIELD(untouched, expectedHash);
			CHECK_NONHASH_UNCHANGED(before, untouched);

			// Second Write is idempotent.
			auto second = writer.Write(std::span<std::byte>{ untouched.data(), untouched.size() });
			CHECK(second.has_value());
			CHECK(untouched == original);
		}
	}

	// ---- Corrected oracle tests (C4 output finalized by Write) ----

	void CorrectedOracleTests()
	{
		// Independently computed by build/shaders/PS49B6A21CI0O1/hash_evidence.py
		// from the reference original (custom-tail MD5 over bytes [20:)).
		constexpr std::array<std::byte, 16> kExpectedCorrectedHash = {
			std::byte{ 0xc6 }, std::byte{ 0xe8 }, std::byte{ 0xde }, std::byte{ 0x1d },
			std::byte{ 0xd3 }, std::byte{ 0x01 }, std::byte{ 0x68 }, std::byte{ 0xfb },
			std::byte{ 0x0b }, std::byte{ 0x4b }, std::byte{ 0xd6 }, std::byte{ 0xeb },
			std::byte{ 0x9b }, std::byte{ 0xa1 }, std::byte{ 0x09 }, std::byte{ 0xfa },
		};

		auto original = NaNBuster::TestFixtures::LoadOriginalShader();

		const DxbcInspector inspector;
		auto view = inspector.Inspect(ByteView{ original.data(), original.size() });
		CHECK(view.has_value());
		if (!view.has_value()) {
			return;
		}

		const NaNBuster::DxbcClampPatcher patcher;
		auto patched = patcher.Patch(*view);
		CHECK(patched.has_value());
		if (!patched.has_value()) {
			return;
		}

		// C4 contract: the corrected container still carries the original hash.
		const Bytes& patchBytes = patched->Bytes;
		CHECK(patchBytes.size() == 1652u);
		CHECK_HASH_FIELD(patchBytes, NaNBuster::OriginalTargetIdentity.EmbeddedHash);

		const NaNBuster::DxbcHashWriter writer;
		Bytes corrected = patchBytes;
		auto writeResult = writer.Write(std::span<std::byte>{ corrected.data(), corrected.size() });
		CHECK(writeResult.has_value());

		CHECK(corrected.size() == 1652u);
		CHECK_HASH_FIELD(corrected, kExpectedCorrectedHash);
		CHECK_NONHASH_UNCHANGED(patchBytes, corrected);
		CHECK(GetU32(corrected, 24) == 1652u);
		CHECK(GetU32(corrected, 264) == 1384u);
		CHECK(GetU32(corrected, 272) == 346u);

		{
			const Bytes first = corrected;
			auto second = writer.Write(std::span<std::byte>{ corrected.data(), corrected.size() });
			CHECK(second.has_value());
			CHECK(corrected == first);
		}
		{
			// The finalized corrected container still validates structurally.
			auto reinspected = inspector.Inspect(ByteView{ corrected.data(), corrected.size() });
			CHECK(reinspected.has_value());
			if (reinspected.has_value()) {
				CHECK(reinspected->Chunks().size() == 3u);
				CHECK(reinspected->Instructions().size() == 55u);
				CHECK(reinspected->ShaderTokenCount() == 344u);
				CHECK(!reinspected->TargetRegion().has_value());
			}
		}
	}

	// ---- Synthetic block-boundary vectors (expected digests from the
	// independent hash_evidence.py tool; all containers carry a zeroed hash
	// field and a single SHEX chunk of `ret` tokens) ----

	struct BoundaryVector
	{
		std::size_t RetCount;
		std::array<std::byte, 16> ExpectedHash;
	};

	constexpr BoundaryVector kBoundaryVectors[] = {
		{ 1, { std::byte{ 0x5b }, std::byte{ 0xde }, std::byte{ 0x81 }, std::byte{ 0x4b },
			   std::byte{ 0x61 }, std::byte{ 0x2e }, std::byte{ 0x65 }, std::byte{ 0x91 },
			   std::byte{ 0xc0 }, std::byte{ 0x37 }, std::byte{ 0xae }, std::byte{ 0xd1 },
			   std::byte{ 0xd1 }, std::byte{ 0xb5 }, std::byte{ 0x57 }, std::byte{ 0x49 } } },
		{ 2, { std::byte{ 0x89 }, std::byte{ 0xc6 }, std::byte{ 0xfd }, std::byte{ 0xac },
			   std::byte{ 0x78 }, std::byte{ 0x67 }, std::byte{ 0x3e }, std::byte{ 0xbc },
			   std::byte{ 0xa6 }, std::byte{ 0xb2 }, std::byte{ 0x81 }, std::byte{ 0x09 },
			   std::byte{ 0x99 }, std::byte{ 0x5f }, std::byte{ 0xa1 }, std::byte{ 0x27 } } },
		{ 3, { std::byte{ 0xc6 }, std::byte{ 0x4b }, std::byte{ 0xbf }, std::byte{ 0x92 },
			   std::byte{ 0x23 }, std::byte{ 0xb5 }, std::byte{ 0x5c }, std::byte{ 0x58 },
			   std::byte{ 0x79 }, std::byte{ 0xaa }, std::byte{ 0x3e }, std::byte{ 0x66 },
			   std::byte{ 0x92 }, std::byte{ 0xd6 }, std::byte{ 0x58 }, std::byte{ 0x6e } } },
		{ 5, { std::byte{ 0x7a }, std::byte{ 0xbb }, std::byte{ 0xc3 }, std::byte{ 0x25 },
			   std::byte{ 0x67 }, std::byte{ 0xa9 }, std::byte{ 0x71 }, std::byte{ 0xcc },
			   std::byte{ 0x8d }, std::byte{ 0xe1 }, std::byte{ 0x4c }, std::byte{ 0xbd },
			   std::byte{ 0x67 }, std::byte{ 0x5e }, std::byte{ 0x87 }, std::byte{ 0x7c } } },
		{ 6, { std::byte{ 0xca }, std::byte{ 0xc8 }, std::byte{ 0x1d }, std::byte{ 0x6b },
			   std::byte{ 0x59 }, std::byte{ 0x36 }, std::byte{ 0xcb }, std::byte{ 0x71 },
			   std::byte{ 0xec }, std::byte{ 0xb6 }, std::byte{ 0x05 }, std::byte{ 0x9e },
			   std::byte{ 0x54 }, std::byte{ 0xb3 }, std::byte{ 0x6b }, std::byte{ 0xa4 } } },
		{ 7, { std::byte{ 0xb5 }, std::byte{ 0x65 }, std::byte{ 0x7c }, std::byte{ 0x58 },
			   std::byte{ 0xaa }, std::byte{ 0x00 }, std::byte{ 0xeb }, std::byte{ 0xe1 },
			   std::byte{ 0x59 }, std::byte{ 0xa4 }, std::byte{ 0x38 }, std::byte{ 0x90 },
			   std::byte{ 0xb1 }, std::byte{ 0xca }, std::byte{ 0x3d }, std::byte{ 0x25 } } },
		{ 8, { std::byte{ 0x1f }, std::byte{ 0x18 }, std::byte{ 0x85 }, std::byte{ 0x5f },
			   std::byte{ 0x51 }, std::byte{ 0x81 }, std::byte{ 0x77 }, std::byte{ 0x39 },
			   std::byte{ 0xc4 }, std::byte{ 0xac }, std::byte{ 0x2c }, std::byte{ 0x10 },
			   std::byte{ 0xc7 }, std::byte{ 0x40 }, std::byte{ 0x3f }, std::byte{ 0xa8 } } },
	};

	void BoundaryVectorTests()
	{
		const NaNBuster::DxbcHashWriter writer;
		const DxbcInspector inspector;

		for (const BoundaryVector& vector : kBoundaryVectors) {
			Bytes bytes = MakeMinimalContainer(vector.RetCount);
			const Bytes before = bytes;

			auto inspected = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			CHECK(inspected.has_value());

			auto result = writer.Write(std::span<std::byte>{ bytes.data(), bytes.size() });
			CHECK(result.has_value());
			CHECK_HASH_FIELD(bytes, vector.ExpectedHash);
			CHECK_NONHASH_UNCHANGED(before, bytes);

			auto again = writer.Write(std::span<std::byte>{ bytes.data(), bytes.size() });
			CHECK(again.has_value());
			CHECK_HASH_FIELD(bytes, vector.ExpectedHash);
		}
	}

	// ---- Malformed inputs reject transactionally ----

	void MalformedInputTests()
	{
		const NaNBuster::DxbcHashWriter writer;

		{
			Bytes garbage(64, std::byte{ 0xAA });
			const Bytes before = garbage;
			auto result = writer.Write(std::span<std::byte>{ garbage.data(), garbage.size() });
			CHECK(!result.has_value());
			if (!result.has_value()) {
				CHECK(result.error().Code == DxbcErrorCode::BadMagic);
				CHECK(result.error().Offset == 0u);
			}
			CHECK(garbage == before);
		}
		{
			auto original = NaNBuster::TestFixtures::LoadOriginalShader();
			Bytes truncated(original.begin(), original.begin() + 100);
			const Bytes before = truncated;
			auto result = writer.Write(std::span<std::byte>{ truncated.data(), truncated.size() });
			CHECK(!result.has_value());
			CHECK(truncated == before);
		}
		{
			Bytes empty;
			auto result = writer.Write(std::span<std::byte>{});
			CHECK(!result.has_value());
		}
	}
}

auto main() -> int
{
	OriginalRestoreTests();
	CorrectedOracleTests();
	BoundaryVectorTests();
	MalformedInputTests();

	if (g_failures != 0) {
		std::cerr << "Core.DxbcHashWriter: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.DxbcHashWriter: all checks passed.\n";
	return EXIT_SUCCESS;
}
