#include "Core/DxbcClampPatcher.h"
#include "Core/DxbcContainer.h"
#include "Core/DxbcInspector.h"
#include "Core/ShaderIdentity.h"
#include "TestShaderFixtures.h"

#include <algorithm>
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
	using NaNBuster::DxbcContainerView;
	using NaNBuster::DxbcError;
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

	void SetU32(Bytes& bytes, std::size_t offset, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) {
			bytes[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
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

	// ---- Synthetic exact-target container builder ----
	// Header carries the exact embedded hash and the total size is exact
	// (1612 unless overridden for rejection tests). Chunks: SHEX(target
	// sequence) then one trailing nonshader chunk "C4TL" padded to size.

	auto BuildExactTargetContainer(
		const std::vector<std::uint32_t>& tokens,
		std::uint32_t totalSize,
		Bytes trailing) -> Bytes
	{
		Bytes bytes;
		bytes.insert(bytes.end(), { std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ 'B' }, std::byte{ 'C' } });
		for (const std::byte b : NaNBuster::OriginalTargetIdentity.EmbeddedHash) {
			bytes.push_back(b);
		}
		PutU32(bytes, 1u); // version
		const std::size_t totalAt = bytes.size();
		PutU32(bytes, 0u); // patched below
		PutU32(bytes, 2u); // chunk count

		Bytes shexContent;
		PutU32(shexContent, 0x50u); // ps_5_0
		PutU32(shexContent, static_cast<std::uint32_t>(tokens.size()) + 2u);
		for (const std::uint32_t token : tokens) {
			PutU32(shexContent, token);
		}

		const std::uint32_t tableEnd = 32u + 2u * 4u;
		const std::uint32_t shexAt = tableEnd;
		const std::uint32_t trailAt = shexAt + 8u + static_cast<std::uint32_t>(shexContent.size());
		PutU32(bytes, shexAt);
		PutU32(bytes, trailAt);

		bytes.insert(bytes.end(), { std::byte{ 'S' }, std::byte{ 'H' }, std::byte{ 'E' }, std::byte{ 'X' } });
		PutU32(bytes, static_cast<std::uint32_t>(shexContent.size()));
		bytes.insert(bytes.end(), shexContent.begin(), shexContent.end());

		trailing.resize(totalSize - (bytes.size() + 8u));
		bytes.insert(bytes.end(), { std::byte{ 'C' }, std::byte{ '4' }, std::byte{ 'T' }, std::byte{ 'L' } });
		PutU32(bytes, static_cast<std::uint32_t>(trailing.size()));
		bytes.insert(bytes.end(), trailing.begin(), trailing.end());

		SetU32(bytes, totalAt, totalSize);
		return bytes;
	}

	// ---- Target sequence tokens (same shapes as the C3 synthetic tests) ----

	constexpr std::uint32_t kRet = 0x0100003Eu;        // ret (opcode 62, len 1)
	constexpr std::uint32_t kLogR0V2 = 0x0500002Fu;    // log r0.xyzw, v2.xyzw
	constexpr std::uint32_t kMulR0Gamma = 0x0A000038u; // mul r0.xyzw, r0.xyzw, l(2.2 x4)
	constexpr std::uint32_t kExpR0 = 0x05000019u;      // exp r0.xyzw, r0.xyzw

	constexpr std::uint32_t kMaxClampOpcode = 0x0A000034u; // max, len 10
	constexpr std::array<std::uint32_t, 10> kInsertedTokens = {
		0x0A000034u,
		0x001000F2u, 0x00000000u,
		0x00101E46u, 0x00000002u,
		0x00004002u,
		0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
	};

	void AppendGammaImmediate(std::vector<std::uint32_t>& tokens)
	{
		tokens.insert(tokens.end(), {
			0x001000F2u, 0x00000000u, // dest r0.xyzw
			0x00100E46u, 0x00000000u, // src r0.xyzw
			0x00004002u,              // imm32, 4 components
			0x400CCCCDu, 0x400CCCCDu, 0x400CCCCDu, 0x400CCCCDu, // 2.2f x4
		});
	}

	auto TargetSequenceTokens() -> std::vector<std::uint32_t>
	{
		std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u };
		tokens.push_back(kMulR0Gamma);
		AppendGammaImmediate(tokens);
		tokens.push_back(kExpR0);
		tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
		tokens.push_back(kRet);
		return tokens;
	}

	// ---- Independent expected-correction oracle (hardcoded offsets/tokens) ----

	auto BuildExpectedOracle(const Bytes& original) -> Bytes
	{
		constexpr std::size_t kInsertionFileOffset = 0x1C8; // original LOG start
		constexpr std::size_t kAfterLogFileOffset = 0x1DC;  // original LOG end
		Bytes expected;
		expected.reserve(1652u);
		expected.insert(expected.end(), original.begin(), original.begin() + kInsertionFileOffset);
		for (const std::uint32_t token : kInsertedTokens) {
			PutU32(expected, token);
		}
		for (std::size_t i = 0; i < 5u; ++i) {
			PutU32(expected, GetU32(original, kInsertionFileOffset + i * 4u));
		}
		// Rewrite only the LOG source operand pair (dwords 3 and 4).
		SetU32(expected, expected.size() - 8u, 0x00100E46u);
		SetU32(expected, expected.size() - 4u, 0x00000000u);
		expected.insert(expected.end(), original.begin() + kAfterLogFileOffset, original.end());
		SetU32(expected, 24u, 1652u);   // declared total
		SetU32(expected, 264u, 1384u);  // SHEX content size
		SetU32(expected, 272u, 346u);   // shader DWORD length
		return expected;
	}

	auto Inspect(ByteView bytes) -> std::expected<DxbcContainerView, DxbcError>
	{
		const DxbcInspector inspector;
		return inspector.Inspect(bytes);
	}

	void ExpectPatchError(const Bytes& bytes, DxbcErrorCode code, std::size_t offset, const char* file, int line)
	{
		auto container = Inspect(ByteView{ bytes.data(), bytes.size() });
		Check(container.has_value(), "rejection input failed to inspect", file, line);
		if (!container.has_value()) {
			return;
		}
		const NaNBuster::DxbcClampPatcher patcher;
		auto result = patcher.Patch(*container);
		Check(!result.has_value(), "non-target input accepted by patcher", file, line);
		if (!result.has_value()) {
			Check(result.error().Code == code, "patch error code", file, line);
			Check(result.error().Offset == offset, "patch error offset", file, line);
			if (result.error().Code != code || result.error().Offset != offset) {
				std::cerr << "  expected code=" << static_cast<int>(code) << " offset=" << offset
					<< " got code=" << static_cast<int>(result.error().Code)
					<< " offset=" << result.error().Offset << '\n';
			}
		}
	}

#define EXPECT_PATCH_ERROR(bytes, code, offset) ExpectPatchError((bytes), (code), (offset), __FILE__, __LINE__)

	// ---- Original oracle patch tests ----

	void OriginalPatchTests()
	{
		constexpr std::size_t kInsertionFileOffset = 0x1C8;
		constexpr std::size_t kRewrittenLogFileOffset = 0x1F0;

		Bytes corrected;
		std::size_t insertionToken = 0;
		std::size_t insertionFile = 0;
		std::size_t logToken = 0;
		std::size_t logFile = 0;
		std::size_t insertedBytes = 0;
		{
			auto original = NaNBuster::TestFixtures::LoadOriginalShader();
			auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
			CHECK(container.has_value());
			if (!container.has_value()) {
				return;
			}
			CHECK(container->TargetRegion().has_value());
			const NaNBuster::DxbcClampPatcher patcher;
			auto result = patcher.Patch(*container);
			CHECK(result.has_value());
			if (!result.has_value()) {
				return;
			}
			insertionToken = result->InsertionTokenOffset;
			insertionFile = result->InsertionFileOffset;
			logToken = result->LogTokenOffset;
			logFile = result->LogFileOffset;
			insertedBytes = result->InsertedByteCount;
			corrected = std::move(result->Bytes);
		} // original storage and all views destroyed here; corrected must stand alone.

		CHECK(corrected.size() == 1652u);
		CHECK(insertionToken == 45u);
		CHECK(insertionFile == 0x1C8u);
		CHECK(logToken == 55u);
		CHECK(logFile == 0x1F0u);
		CHECK(insertedBytes == 40u);

		// Independent hardcoded-oracle comparison of the complete result.
		const Bytes original = NaNBuster::TestFixtures::LoadOriginalShader();
		const Bytes expected = BuildExpectedOracle(original);
		CHECK(expected.size() == 1652u);
		CHECK(std::equal(corrected.begin(), corrected.end(), expected.begin(), expected.end()));

		// Header and chunk-table fields.
		CHECK(GetU32(corrected, 24u) == 1652u);
		CHECK(GetU32(corrected, 32u) == 44u);
		CHECK(GetU32(corrected, 36u) == 208u);
		CHECK(GetU32(corrected, 40u) == 260u);
		CHECK(GetU32(corrected, 260u) == 0x58454853u); // "SHEX"
		CHECK(GetU32(corrected, 264u) == 1384u);
		CHECK(GetU32(corrected, 268u) == GetU32(original, 268u)); // SHEX version unchanged
		CHECK(GetU32(corrected, 272u) == 346u);

		// Embedded hash preserved byte-for-byte for C5.
		for (std::size_t i = 0; i < 16u; ++i) {
			CHECK(corrected[4u + i] == original[4u + i]);
			CHECK(corrected[4u + i] == NaNBuster::OriginalTargetIdentity.EmbeddedHash[i]);
		}

		// Inserted MAX instruction encoding (ten DWORDs at 0x1C8).
		for (std::size_t i = 0; i < kInsertedTokens.size(); ++i) {
			CHECK(GetU32(corrected, kInsertionFileOffset + i * 4u) == kInsertedTokens[i]);
		}

		// Rewritten LOG: opcode/dest unchanged, source now r0.xyzw.
		CHECK(GetU32(corrected, kRewrittenLogFileOffset) == GetU32(original, kInsertionFileOffset));
		CHECK(GetU32(corrected, kRewrittenLogFileOffset + 4u) == 0x001000F2u);
		CHECK(GetU32(corrected, kRewrittenLogFileOffset + 8u) == 0x00000000u);
		CHECK(GetU32(corrected, kRewrittenLogFileOffset + 12u) == 0x00100E46u);
		CHECK(GetU32(corrected, kRewrittenLogFileOffset + 16u) == 0x00000000u);
		CHECK(GetU32(original, kInsertionFileOffset + 12u) == 0x00101E46u);
		CHECK(GetU32(original, kInsertionFileOffset + 16u) == 0x00000002u);

		// Unchanged-remainder sweep: everything outside the declared header
		// fields, the inserted block, and the LOG source pair maps 1:1.
		const auto headerByteChanged = [](std::size_t i) {
			return (i >= 24u && i < 28u) || (i >= 264u && i < 268u) || (i >= 272u && i < 276u);
		};
		for (std::size_t i = 0; i < kInsertionFileOffset; ++i) {
			if (!headerByteChanged(i)) {
				CHECK(corrected[i] == original[i]);
			}
		}
		for (std::size_t i = kRewrittenLogFileOffset + 20u; i < corrected.size(); ++i) {
			CHECK(corrected[i] == original[i - 40u]);
		}
		// Resource-interface payloads byte-identical.
		for (std::size_t i = 52u; i < 208u; ++i) { // ISGN content
			CHECK(corrected[i] == original[i]);
		}
		for (std::size_t i = 216u; i < 260u; ++i) { // OSGN content
			CHECK(corrected[i] == original[i]);
		}
		// Shifted sequence: corrected MUL@0x204 == original MUL@0x1DC, EXP@0x22C == original @0x204.
		for (std::size_t i = 0x204u; i < 0x22Cu; ++i) {
			CHECK(corrected[i] == original[i - 40u]);
		}

		// Reinspect the owned corrected bytes.
		auto recheck = Inspect(corrected);
		CHECK(recheck.has_value());
		if (recheck.has_value()) {
			CHECK(recheck->OriginalBytes().size() == 1652u);
			CHECK(recheck->Chunks().size() == 3u);
			CHECK(recheck->Chunks()[0].Offset == 44u && recheck->Chunks()[0].Size == 156u);
			CHECK(recheck->Chunks()[1].Offset == 208u && recheck->Chunks()[1].Size == 44u);
			CHECK(recheck->Chunks()[2].Offset == 260u && recheck->Chunks()[2].Size == 1384u);
			CHECK(recheck->ShaderStreamFileOffset() == 0x114u);
			CHECK(recheck->ShaderTokenCount() == 344u);
			CHECK(recheck->Instructions().size() == 55u);
			CHECK(recheck->Instructions()[14].TokenOffset == 45u);
			CHECK(recheck->Instructions()[14].TokenLength == 10u);
			CHECK(recheck->Instructions()[14].Opcode == 52u); // MAX
			CHECK(recheck->Instructions()[15].TokenOffset == 55u);
			CHECK(recheck->Instructions()[15].TokenLength == 5u);
			CHECK(recheck->Instructions()[15].Opcode == 47u); // LOG
			CHECK(recheck->Instructions()[16].TokenOffset == 60u);
			CHECK(recheck->Instructions()[16].TokenLength == 10u);
			CHECK(recheck->Instructions()[16].Opcode == 56u); // MUL
			CHECK(recheck->Instructions()[17].TokenOffset == 70u);
			CHECK(recheck->Instructions()[17].TokenLength == 5u);
			CHECK(recheck->Instructions()[17].Opcode == 25u); // EXP
			CHECK(recheck->Instructions().back().TokenOffset == 343u);
			CHECK(recheck->Instructions().back().Opcode == 62u); // RET
			CHECK(!recheck->TargetRegion().has_value());
		}
	}

	// ---- Synthetic exact-size/hash target with a trailing chunk ----

	void TrailingChunkShiftTests()
	{
		auto original = BuildExactTargetContainer(TargetSequenceTokens(), 1612u, Bytes(4u, std::byte{ 0x77 }));
		CHECK(original.size() == 1612u);
		const std::uint32_t trailTableOriginal = GetU32(original, 36u);
		const std::uint32_t shexSizeOriginal = GetU32(original, 44u);
		const std::uint32_t shexLengthOriginal = GetU32(original, 52u);
		CHECK(GetU32(original, 32u) == 40u); // SHEX table entry
		CHECK(trailTableOriginal == 140u);
		CHECK(shexSizeOriginal == 92u);
		CHECK(shexLengthOriginal == 23u);
		CHECK(GetU32(original, 48u) == 0x50u); // SHEX version

		auto container = Inspect(ByteView{ original.data(), original.size() });
		CHECK(container.has_value());
		if (!container.has_value()) {
			return;
		}
		CHECK(container->TargetRegion().has_value());
		CHECK(container->TargetRegion()->TokenOffset == 0u);

		const NaNBuster::DxbcClampPatcher patcher;
		auto result = patcher.Patch(*container);
		CHECK(result.has_value());
		if (!result.has_value()) {
			return;
		}
		const Bytes& corrected = result->Bytes;

		CHECK(corrected.size() == 1652u);
		CHECK(result->InsertionTokenOffset == 0u);
		CHECK(result->InsertionFileOffset == 56u);
		CHECK(result->LogTokenOffset == 10u);
		CHECK(result->LogFileOffset == 96u);
		CHECK(result->InsertedByteCount == 40u);

		CHECK(GetU32(corrected, 24u) == 1652u);
		CHECK(GetU32(corrected, 32u) == 40u);                        // preceding offset unchanged
		CHECK(GetU32(corrected, 36u) == trailTableOriginal + 40u);   // trailing offset advanced
		CHECK(GetU32(corrected, 44u) == shexSizeOriginal + 40u);
		CHECK(GetU32(corrected, 52u) == shexLengthOriginal + 10u);
		CHECK(GetU32(corrected, 48u) == 0x50u);

		// Trailing chunk payload byte-identical, shifted by 40.
		for (std::size_t i = 0; i < 1612u - (trailTableOriginal + 8u); ++i) {
			CHECK(corrected[trailTableOriginal + 40u + 8u + i] == original[trailTableOriginal + 8u + i]);
		}
		// Preceding bytes outside the patched header fields unchanged (the
		// trailing chunk's table entry at 36 advances; 24/44/52 are the
		// declared-total, SHEX-size, and shader-length fields).
		for (std::size_t i = 0; i < 56u; ++i) {
			const bool changed = (i >= 24u && i < 28u) || (i >= 36u && i < 40u) ||
				(i >= 44u && i < 48u) || (i >= 52u && i < 56u);
			if (!changed) {
				CHECK(corrected[i] == original[i]);
			}
		}
		// Inserted clamp at the stream start.
		for (std::size_t i = 0; i < kInsertedTokens.size(); ++i) {
			CHECK(GetU32(corrected, 56u + i * 4u) == kInsertedTokens[i]);
		}

		auto recheck = Inspect(corrected);
		CHECK(recheck.has_value());
		if (recheck.has_value()) {
			CHECK(recheck->Chunks().size() == 2u);
			CHECK(recheck->Chunks()[0].Offset == 40u && recheck->Chunks()[0].Size == 132u);
			CHECK(recheck->Chunks()[1].Offset == trailTableOriginal + 40u);
			CHECK(recheck->Chunks()[1].Size == 1612u - (trailTableOriginal + 8u));
			CHECK(recheck->Instructions().size() == 5u);
			CHECK(recheck->Instructions()[0].Opcode == 52u && recheck->Instructions()[0].TokenLength == 10u);
			CHECK(recheck->Instructions()[1].Opcode == 47u && recheck->Instructions()[1].TokenOffset == 10u);
			CHECK(!recheck->TargetRegion().has_value());
		}
	}

	// ---- Rejection matrix (descriptive offset-bearing errors, no throw) ----

	void RejectionTests()
	{
		{
			// Wrong embedded hash (one byte flipped), exact size.
			auto bytes = BuildExactTargetContainer(TargetSequenceTokens(), 1612u, Bytes(4u, std::byte{ 0x77 }));
			bytes[5u] ^= std::byte{ 0xFF };
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::HashMismatch, 4u);
		}
		{
			// Wrong size: identical container padded to 1616.
			auto bytes = BuildExactTargetContainer(TargetSequenceTokens(), 1616u, Bytes(4u, std::byte{ 0x77 }));
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::TargetSizeMismatch, 24u);
		}
		{
			// Missing sequence: exact size/hash, no log/mul/exp.
			EXPECT_PATCH_ERROR(
				BuildExactTargetContainer({ kRet }, 1612u, Bytes(4u, std::byte{ 0x77 })),
				DxbcErrorCode::TargetRegionMissing, 0u);
		}
		{
			// Wrong register: log r0.xyzw, v1.xyzw.
			const std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000001u };
			auto bytes = BuildExactTargetContainer(tokens, 1612u, Bytes(4u, std::byte{ 0x77 }));
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::TargetRegionMissing, 0u);
		}
		{
			// Wrong mask: log destination xyz instead of xyzw.
			const std::vector<std::uint32_t> tokens = { kLogR0V2, 0x00100072u, 0x00000000u, 0x00101E46u, 0x00000002u };
			auto bytes = BuildExactTargetContainer(tokens, 1612u, Bytes(4u, std::byte{ 0x77 }));
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::TargetRegionMissing, 0u);
		}
		{
			// Scrambled order: log, exp, mul.
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u };
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			tokens.push_back(kMulR0Gamma);
			AppendGammaImmediate(tokens);
			tokens.push_back(kRet);
			auto bytes = BuildExactTargetContainer(tokens, 1612u, Bytes(4u, std::byte{ 0x77 }));
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::TargetRegionMissing, 0u);
		}
		{
			// Already clamped: canonical MAX insertion plus rewritten LOG source.
			std::vector<std::uint32_t> tokens(kInsertedTokens.begin(), kInsertedTokens.end());
			tokens.insert(tokens.end(), { 0x0500002Fu, 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			tokens.push_back(kMulR0Gamma);
			AppendGammaImmediate(tokens);
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			tokens.push_back(kRet);
			auto bytes = BuildExactTargetContainer(tokens, 1612u, Bytes(4u, std::byte{ 0x77 }));
			EXPECT_PATCH_ERROR(bytes, DxbcErrorCode::AlreadyPatched, 0u);
		}
		{
			// Malformed bytes stay at the Inspect boundary and never reach Patch.
			const Bytes garbage(64u, std::byte{ 0xAA });
			auto container = Inspect(ByteView{ garbage.data(), garbage.size() });
			CHECK(!container.has_value());
			if (!container.has_value()) {
				CHECK(container.error().Code == DxbcErrorCode::BadMagic);
			}
		}
	}
}

auto main() -> int
{
	OriginalPatchTests();
	TrailingChunkShiftTests();
	RejectionTests();

	if (g_failures != 0) {
		std::cerr << "Core.DxbcClampPatcher: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.DxbcClampPatcher: all checks passed.\n";
	return EXIT_SUCCESS;
}
