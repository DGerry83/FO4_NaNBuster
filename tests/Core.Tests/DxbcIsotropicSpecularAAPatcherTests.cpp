#include "Core/DxbcHashWriter.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcIsotropicSpecularAAPatcher.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "Core/ShaderIdentityMatcher.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_ISOTROPIC_ORIGINAL_PATH
#	error "NANBUSTER_ISOTROPIC_ORIGINAL_PATH is required."
#endif
#ifndef NANBUSTER_ORIGINAL_SHADER_PATH
#	error "NANBUSTER_ORIGINAL_SHADER_PATH is required."
#endif

namespace
{
	using Bytes = std::vector<std::byte>;
	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

	auto GetU32(const Bytes& bytes, std::size_t offset) -> std::uint32_t
	{
		return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset])) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset + 1])) << 8) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset + 2])) << 16) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(bytes[offset + 3])) << 24);
	}

	void SetU32(Bytes& bytes, std::size_t offset, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) {
			bytes[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
		}
	}

	auto Token(const NaNBuster::DxbcContainerView& container, std::size_t offset) -> std::uint32_t
	{
		const auto& stream = container.ShaderStream();
		const std::size_t at = offset * 4;
		return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at])) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 1])) << 8) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 2])) << 16) |
			(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[at + 3])) << 24);
	}

	auto Inspect(NaNBuster::ByteView bytes) -> std::expected<NaNBuster::DxbcContainerView, NaNBuster::DxbcError>
	{
		return NaNBuster::DxbcInspector{}.Inspect(bytes);
	}

	auto TempCount(const NaNBuster::DxbcContainerView& container) -> std::uint32_t
	{
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode == 104) {
				return Token(container, instruction.TokenOffset + 1);
			}
		}
		return 0;
	}

	void CheckRemainder(const Bytes& original, const Bytes& corrected)
	{
		constexpr std::size_t variance = 1668;
		constexpr std::size_t exponent = 4300;
		constexpr std::size_t varianceBytes = 404;
		constexpr std::size_t totalInserted = 544;
		for (std::size_t i = 0; i < original.size(); ++i) {
			const bool header = (i >= 24 && i < 28) || (i >= 176 && i < 180) || (i >= 184 && i < 188);
			if (header) {
				continue;
			}
			const std::size_t correctedOffset = i < variance ? i : i < exponent ? i + varianceBytes : i + totalInserted;
			CHECK(corrected[correctedOffset] == original[i]);
		}
	}

	void CheckDecodeClone(const NaNBuster::DxbcContainerView& original, const Bytes& corrected)
	{
		constexpr std::size_t originalOffsets[] = { 403, 418, 425, 441, 446, 453 };
		constexpr std::size_t lengths[] = { 15, 7, 16, 5, 7, 6 };
		std::size_t correctedToken = 370;
		for (std::size_t instruction = 0; instruction < 6; ++instruction) {
			for (std::size_t i = 0; i < lengths[instruction]; ++i) {
				const std::uint32_t expected = Token(original, originalOffsets[instruction] + i);
				const std::uint32_t actual = GetU32(corrected, 188 + correctedToken * 4);
				switch (instruction) {
				case 0:
					if (i == 2) CHECK(actual == 8);
					else CHECK(actual == expected);
					break;
				case 1:
					if (i == 2) CHECK(actual == 4);
					else if (i == 4 || i == 6) CHECK(actual == 8);
					else CHECK(actual == expected);
					break;
				case 2:
					if (i == 5) CHECK(actual == 4);
					else CHECK(actual == expected);
					break;
				case 3:
					if (i == 1) CHECK(actual == 0x00100022u);
					else if (i == 2) CHECK(actual == 4);
					else CHECK(actual == expected);
					break;
				case 4:
					if (i == 2) CHECK(actual == 8);
					else if (i == 3) CHECK(actual == 0x00100556u);
					else if (i == 4) CHECK(actual == 4);
					else if (i == 6) CHECK(actual == 8);
					else CHECK(actual == expected);
					break;
				case 5:
					if (i == 2) CHECK(actual == 8);
					else CHECK(actual == expected);
					break;
				}
				++correctedToken;
			}
		}
		CHECK(correctedToken == 426);
	}

	void CheckAddedTokens(const Bytes& corrected)
	{
		constexpr std::array<std::uint32_t, 45> variance{
			0x0500007bu, 0x00100072u, 5, 0x00100246u, 8,
			0x0500007du, 0x00100072u, 6, 0x00100246u, 8,
			0x07000010u, 0x00100082u, 5, 0x00100246u, 5, 0x00100246u, 5,
			0x07000010u, 0x00100082u, 6, 0x00100246u, 6, 0x00100246u, 6,
			0x07000000u, 0x00100082u, 5, 0x0010003au, 5, 0x0010003au, 6,
			0x07000038u, 0x00100082u, 5, 0x0010003au, 5, 0x00004001u, 0x3E800000u,
			0x07000033u, 0x00100082u, 8, 0x0010003au, 5, 0x00004001u, 0x3CA3D70Au,
		};
		for (std::size_t i = 0; i < variance.size(); ++i) {
			const auto actual = GetU32(corrected, 188 + (426 + i) * 4);
			if (actual != variance[i]) {
				std::cerr << "variance token " << i << ": actual 0x" << std::hex << actual << " expected 0x" << variance[i] << std::dec << '\n';
			}
			CHECK(actual == variance[i]);
		}

		constexpr std::array<std::uint32_t, 35> exponent{
			0x07000000u, 0x00100012u, 6, 0x0010000au, 3, 0x00004001u, 0x40000000u,
			0x0700000eu, 0x00100012u, 6, 0x00004001u, 0x40000000u, 0x0010000au, 6,
			0x07002000u, 0x00100012u, 6, 0x0010000au, 6, 0x0010003au, 8,
			0x0700000eu, 0x00100012u, 6, 0x00004001u, 0x40000000u, 0x0010000au, 6,
			0x07000000u, 0x00100012u, 3, 0x0010000au, 6, 0x00004001u, 0xC0000000u,
		};
		for (std::size_t i = 0; i < exponent.size(); ++i) {
			const auto actual = GetU32(corrected, 188 + (1129 + i) * 4);
			if (actual != exponent[i]) {
				std::cerr << "exponent token " << i << ": actual 0x" << std::hex << actual << " expected 0x" << exponent[i] << std::dec << '\n';
			}
			CHECK(actual == exponent[i]);
		}
	}
}

auto main() -> int
{
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH);
	CHECK(original.size() == 6528);
	CHECK(NaNBuster::ShaderIdentityMatcher::Matches(
		NaNBuster::TestFixtures::AsByteView(original), NaNBuster::IsotropicSpecularTargetIdentity));
	auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
	CHECK(container.has_value());
	if (!container) {
		return EXIT_FAILURE;
	}
	CHECK(container->ShaderTokenCount() == 1585);
	CHECK(TempCount(*container) == 9);
	for (std::size_t i = 0; i + 8 <= original.size(); i += 4) {
		CHECK(!(GetU32(original, static_cast<std::size_t>(i)) == 0x00100082u &&
			GetU32(original, i + 4) == 8));
	}

	NaNBuster::DxbcIsotropicSpecularAAPatcher patcher;
	auto result = patcher.Patch(*container);
	CHECK(result.has_value());
	if (!result) {
		std::cerr << "patch error code=" << static_cast<int>(result.error().Code) << " offset=" << result.error().Offset << '\n';
		return EXIT_FAILURE;
	}
	CHECK(result->Bytes.size() == 7072);
	CHECK(result->VarianceInsertionTokenOffset == 370);
	CHECK(result->VarianceInsertionFileOffset == 1668);
	CHECK(result->ExponentOriginalTokenOffset == 1028);
	CHECK(result->ExponentOriginalFileOffset == 4300);
	CHECK(result->InsertedTokenCount == 136);
	CHECK(result->OriginalTempCount == 9);
	CHECK(GetU32(result->Bytes, 24) == 7072);
	CHECK(GetU32(result->Bytes, 176) == 6892);
	CHECK(GetU32(result->Bytes, 184) == 1723);

	auto corrected = Inspect(NaNBuster::TestFixtures::AsByteView(result->Bytes));
	CHECK(corrected.has_value());
	if (!corrected) {
		return EXIT_FAILURE;
	}
	{
		CHECK(corrected->Instructions().size() == container->Instructions().size() + 18);
		CHECK(corrected->ShaderTokenCount() == 1721);
		CHECK(TempCount(*corrected) == 9);
	}
	CheckRemainder(original, result->Bytes);
	CheckDecodeClone(*container, result->Bytes);
	CheckAddedTokens(result->Bytes);

	auto repeat = patcher.Patch(*container);
	CHECK(repeat.has_value() && repeat->Bytes == result->Bytes);
	auto already = patcher.Patch(*corrected);
	CHECK(!already && already.error().Code == NaNBuster::DxbcErrorCode::AlreadyPatched);

	auto wrongHash = original;
	wrongHash[4] ^= std::byte{ 0xFF };
	auto wrongHashContainer = Inspect(NaNBuster::TestFixtures::AsByteView(wrongHash));
	CHECK(wrongHashContainer.has_value());
	auto rejectedHash = patcher.Patch(*wrongHashContainer);
	CHECK(!rejectedHash && rejectedHash.error().Code == NaNBuster::DxbcErrorCode::HashMismatch);

	auto wrongAnchor = original;
	SetU32(wrongAnchor, 188 + 370 * 4, 0x03000000u);
	auto wrongAnchorContainer = Inspect(NaNBuster::TestFixtures::AsByteView(wrongAnchor));
	CHECK(wrongAnchorContainer.has_value());
	auto rejectedAnchor = patcher.Patch(*wrongAnchorContainer);
	CHECK(!rejectedAnchor && rejectedAnchor.error().Code == NaNBuster::DxbcErrorCode::TargetRegionMissing);

	const auto other = NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ORIGINAL_SHADER_PATH);
	auto otherContainer = Inspect(NaNBuster::TestFixtures::AsByteView(other));
	CHECK(otherContainer.has_value());
	auto rejectedSize = patcher.Patch(*otherContainer);
	CHECK(!rejectedSize && rejectedSize.error().Code == NaNBuster::DxbcErrorCode::TargetSizeMismatch);

	NaNBuster::DxbcHashWriter hashWriter;
	auto finalized = result->Bytes;
	CHECK(hashWriter.Write(finalized).has_value());
	CHECK(finalized.size() == 7072);
	auto finalizedContainer = Inspect(NaNBuster::TestFixtures::AsByteView(finalized));
	CHECK(finalizedContainer.has_value());
	CHECK(std::memcmp(original.data() + 4, finalized.data() + 4, 16) != 0);

	if (g_failures != 0) {
		std::cerr << "Core.DxbcIsotropicSpecularAAPatcher: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Core.DxbcIsotropicSpecularAAPatcher: all checks passed (7072 bytes, 136 tokens, 18 instructions, exact remainder/rejection/determinism).\n";
	return EXIT_SUCCESS;
}
