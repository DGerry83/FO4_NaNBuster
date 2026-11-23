#include "Core/DxbcLambertGuardPatcher.h"
#include "Core/DxbcContainer.h"
#include "Core/DxbcInspector.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

#ifndef NANBUSTER_LAMBERT_REFERENCE_ROOT
#	error "NANBUSTER_LAMBERT_REFERENCE_ROOT must point to the read-only Lambert reference root."
#endif

namespace
{
	using Bytes = std::vector<std::byte>;
	using NaNBuster::ByteView;
	using NaNBuster::DxbcChunkDescriptor;
	using NaNBuster::DxbcContainerView;
	using NaNBuster::DxbcErrorCode;
	using NaNBuster::DxbcInstructionDescriptor;

	constexpr std::size_t kInsertedBytes = 42 * sizeof(std::uint32_t);
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

	void PutU32(Bytes& bytes, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) {
			bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFu));
		}
	}

	auto Inspect(ByteView bytes) -> std::expected<DxbcContainerView, NaNBuster::DxbcError>
	{
		return NaNBuster::DxbcInspector{}.Inspect(bytes);
	}

	auto ShaderChunk(const DxbcContainerView& container) -> const DxbcChunkDescriptor&
	{
		return *std::find_if(container.Chunks().begin(), container.Chunks().end(), [](const auto& chunk) {
			return chunk.FourCC == 0x58454853u || chunk.FourCC == 0x52444853u;
		});
	}

	auto Temps(const DxbcContainerView& container) -> std::pair<std::size_t, std::uint32_t>
	{
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode == 104) {
				return { instruction.TokenOffset, GetU32(Bytes(
					container.ShaderStream().begin(), container.ShaderStream().end()), (instruction.TokenOffset + 1) * 4) };
			}
		}
		return { 0, 0 };
	}

	auto FindDecode(const DxbcContainerView& container) -> std::size_t
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 5 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 50 && instructions[i + 1].Opcode == 15 &&
				instructions[i + 2].Opcode == 50 && instructions[i + 3].Opcode == 75 &&
				instructions[i + 4].Opcode == 56 &&
				(instructions[i + 5].Opcode == 54 || instructions[i + 5].Opcode == 56)) {
				return i;
			}
		}
		return instructions.size();
	}

	bool HasGuard(const DxbcContainerView& container)
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 5 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 49 && instructions[i + 1].Opcode == 68 &&
				instructions[i + 2].Opcode == 56 && instructions[i + 3].Opcode == 55 &&
				instructions[i + 4].Opcode == 56 && instructions[i + 5].Opcode == 15) {
				return true;
			}
		}
		return false;
	}

	auto InsertRawTokens(const Bytes& original, std::size_t insertionTokenOffset, const std::vector<std::uint32_t>& inserted) -> Bytes
	{
		auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
		if (!container) {
			return {};
		}
		const auto& shader = ShaderChunk(*container);
		const std::size_t insertionFile = container->ShaderStreamFileOffset() + insertionTokenOffset * 4;
		const std::size_t insertedBytes = inserted.size() * 4;
		Bytes result;
		result.reserve(original.size() + insertedBytes);
		result.insert(result.end(), original.begin(), original.begin() + insertionFile);
		for (const auto token : inserted) {
			PutU32(result, token);
		}
		result.insert(result.end(), original.begin() + insertionFile, original.end());
		SetU32(result, 24, static_cast<std::uint32_t>(result.size()));
		for (std::size_t i = 0; i < container->Chunks().size(); ++i) {
			if (container->Chunks()[i].Offset > shader.Offset) {
				SetU32(result, 32 + i * 4, GetU32(result, 32 + i * 4) + static_cast<std::uint32_t>(insertedBytes));
			}
		}
		SetU32(result, shader.Offset + 4, shader.Size + static_cast<std::uint32_t>(insertedBytes));
		SetU32(result, shader.Offset + 12,
			container->ShaderTokenCount() + 2 + static_cast<std::uint32_t>(inserted.size()));
		return result;
	}

	std::vector<std::uint32_t> ExpectedGuard(const DxbcContainerView& container, std::size_t decodeIndex, std::uint32_t tempCount)
	{
		const auto instructions = container.Instructions();
		const auto& mad = instructions[decodeIndex];
		const auto& dp2 = instructions[decodeIndex + 1];
		const auto& stream = container.ShaderStream();
		auto token = [&stream](std::size_t offset) {
			return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset * 4])) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset * 4 + 1])) << 8) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset * 4 + 2])) << 16) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(stream[offset * 4 + 3])) << 24);
		};
		const std::uint32_t fToken = token(dp2.TokenOffset + 1);
		const std::uint32_t fRegister = token(dp2.TokenOffset + 2);
		const std::uint32_t fMask = (fToken >> 4) & 0x0Fu;
		const std::uint32_t fComponent = fMask == 1 ? 0 : fMask == 2 ? 1 : fMask == 4 ? 2 : 3;
		const std::uint32_t fSource = 0x00100006u | ((fComponent * 0x55u) << 4);
		const std::uint32_t mask = tempCount;
		const std::uint32_t scale = tempCount + 1;
		const std::uint32_t eDestToken = token(mad.TokenOffset + 1);
		const std::uint32_t eDestRegister = token(mad.TokenOffset + 2);
		const std::uint32_t eToken = token(dp2.TokenOffset + 3);
		const std::uint32_t eRegister = token(dp2.TokenOffset + 4);
		const std::uint32_t fDestToken = token(dp2.TokenOffset + 1);

		std::vector<std::uint32_t> expected{
			0x07000031u, 0x00100012u, mask, 0x00004001u, 0x40800000u, fSource, fRegister,
			0x05000044u, 0x00100012u, scale, fSource, fRegister,
			0x07000038u, 0x00100012u, scale, 0x00100006u, scale, 0x00004001u, 0x3FFFDF3Bu,
			0x09000037u, 0x00100012u, scale, 0x00100006u, mask, 0x00100006u, scale, 0x00004001u, 0x3F800000u,
			0x07000038u, eDestToken, eDestRegister, eToken, eRegister, 0x00100006u, scale,
			0x0700000Fu, fDestToken, fRegister, eToken, eRegister, eToken, eRegister,
		};
		return expected;
	}

	void CheckRemainder(
		const Bytes& original,
		const Bytes& corrected,
		const DxbcContainerView& originalContainer,
		std::size_t insertionFile,
		std::size_t tempsTokenOffset)
	{
		const auto& shader = ShaderChunk(originalContainer);
		auto allowed = [&](std::size_t offset) {
			if ((offset >= 24 && offset < 28) ||
				(offset >= shader.Offset + 4 && offset < shader.Offset + 16) ||
				(offset >= originalContainer.ShaderStreamFileOffset() + tempsTokenOffset * 4 + 4 &&
					offset < originalContainer.ShaderStreamFileOffset() + tempsTokenOffset * 4 + 8)) {
				return true;
			}
			for (std::size_t i = 0; i < originalContainer.Chunks().size(); ++i) {
				if (originalContainer.Chunks()[i].Offset > shader.Offset && offset >= 32 + i * 4 && offset < 36 + i * 4) {
					return true;
				}
			}
			return false;
		};
		for (std::size_t i = 0; i < insertionFile; ++i) {
			if (!allowed(i)) {
				CHECK(corrected[i] == original[i]);
			}
		}
		for (std::size_t i = insertionFile; i < original.size(); ++i) {
			if (!allowed(i)) {
				CHECK(corrected[i + kInsertedBytes] == original[i]);
			}
		}
	}

	void RealTargetTests()
	{
		const std::filesystem::path root = NANBUSTER_LAMBERT_REFERENCE_ROOT;
		const NaNBuster::DxbcLambertGuardPatcher patcher;
		for (const auto& identity : NaNBuster::LambertShaderIdentities) {
			const auto original = NaNBuster::TestFixtures::LoadBinaryFile(
				(root / identity.ReferenceStem / "original.dxbc").string());
			auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
			CHECK(container.has_value());
			if (!container) {
				continue;
			}
			const std::size_t decodeIndex = FindDecode(*container);
			CHECK(decodeIndex != container->Instructions().size());
			const auto [tempsOffset, tempCount] = Temps(*container);
			CHECK(tempCount > 0);
			auto result = patcher.Patch(*container, identity);
			CHECK(result.has_value());
			if (!result) {
				std::cerr << identity.Identity.ShaderUid << " patch error code=" << static_cast<int>(result.error().Code)
						  << " offset=" << result.error().Offset << '\n';
				continue;
			}
			CHECK(result->Bytes.size() == original.size() + kInsertedBytes);
			CHECK(result->InsertedTokenCount == 42);
			CHECK(result->OriginalTempCount == tempCount);
			CHECK(result->InsertionTokenOffset == container->Instructions()[decodeIndex + 2].TokenOffset);

			auto correctedContainer = Inspect(NaNBuster::TestFixtures::AsByteView(result->Bytes));
			CHECK(correctedContainer.has_value());
			if (!correctedContainer) {
				continue;
			}
			CHECK(correctedContainer->Instructions().size() == container->Instructions().size() + 6);
			CHECK(correctedContainer->ShaderTokenCount() == container->ShaderTokenCount() + 42);
			CHECK(Temps(*correctedContainer).second == tempCount + 2);
			CHECK(HasGuard(*correctedContainer));

			const auto expected = ExpectedGuard(*container, decodeIndex, tempCount);
			CHECK(expected.size() == 42);
			Bytes expectedBytes;
			for (const auto token : expected) {
				PutU32(expectedBytes, token);
			}
			const auto actualGuardBegin = result->Bytes.begin() + static_cast<std::ptrdiff_t>(result->InsertionFileOffset);
			if (!std::equal(expectedBytes.begin(), expectedBytes.end(), actualGuardBegin)) {
				for (std::size_t tokenIndex = 0; tokenIndex < 42; ++tokenIndex) {
					const auto expectedToken = GetU32(expectedBytes, tokenIndex * 4);
					const auto actualToken = GetU32(result->Bytes, result->InsertionFileOffset + tokenIndex * 4);
					if (expectedToken != actualToken) {
						std::cerr << identity.Identity.ShaderUid << " token " << tokenIndex << " expected="
								  << std::hex << expectedToken << " actual=" << actualToken << std::dec << '\n';
					}
				}
			}
			CHECK(std::equal(expectedBytes.begin(), expectedBytes.end(), actualGuardBegin));
			CheckRemainder(original, result->Bytes, *container, result->InsertionFileOffset, tempsOffset);

			auto second = patcher.Patch(*container, identity);
			CHECK(second.has_value());
			if (second) {
				CHECK(second->Bytes == result->Bytes);
			}
			auto wrongSize = patcher.Patch(*correctedContainer, identity);
			CHECK(!wrongSize.has_value());
			if (!wrongSize) {
				CHECK(wrongSize.error().Code == DxbcErrorCode::TargetSizeMismatch);
			}
			auto guardIdentity = identity;
			guardIdentity.Identity.BytecodeSize = static_cast<std::uint32_t>(result->Bytes.size());
			auto already = patcher.Patch(*correctedContainer, guardIdentity);
			CHECK(!already.has_value());
			if (!already) {
				CHECK(already.error().Code == DxbcErrorCode::AlreadyPatched);
			}
		}
	}

	void RejectionTests()
	{
		const std::filesystem::path path = std::filesystem::path(NANBUSTER_LAMBERT_REFERENCE_ROOT) /
			NaNBuster::LambertShaderIdentities[0].ReferenceStem / "original.dxbc";
		const auto original = NaNBuster::TestFixtures::LoadBinaryFile(path.string());
		const auto& identity = NaNBuster::LambertShaderIdentities[0];
		const NaNBuster::DxbcLambertGuardPatcher patcher;

		auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
		CHECK(container.has_value());
		if (!container) {
			return;
		}
		const std::size_t decodeIndex = FindDecode(*container);
		CHECK(decodeIndex != container->Instructions().size());

		{
			auto wrong = original;
			wrong[4] ^= std::byte{ 0xFF };
			auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
			auto result = patcher.Patch(*inspected, identity);
			CHECK(!result && result.error().Code == DxbcErrorCode::HashMismatch);
		}
		{
			auto wrong = original;
			SetU32(wrong, ShaderChunk(*container).Offset + 8, 0x00050050u);
			auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
			auto result = patcher.Patch(*inspected, identity);
			CHECK(!result && result.error().Code == DxbcErrorCode::TargetRegionMissing);
		}
		{
			auto wrong = original;
			const auto& dp2 = container->Instructions()[decodeIndex + 1];
			SetU32(wrong, container->ShaderStreamFileOffset() + (dp2.TokenOffset + 4) * 4, 999);
			auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
			auto result = patcher.Patch(*inspected, identity);
			CHECK(!result && result.error().Code == DxbcErrorCode::TargetRegionMissing);
		}
		{
			const auto [tempsOffset, tempCount] = Temps(*container);
			auto wrong = InsertRawTokens(original, container->Instructions()[decodeIndex].TokenOffset, { 0x02000068u, tempCount });
			auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
			CHECK(inspected.has_value());
			auto expandedIdentity = identity;
			expandedIdentity.Identity.BytecodeSize = static_cast<std::uint32_t>(wrong.size());
			auto result = patcher.Patch(*inspected, expandedIdentity);
			CHECK(!result && result.error().Code == DxbcErrorCode::TargetRegionMissing);
			(void)tempsOffset;
		}
		{
			const auto instructions = container->Instructions();
			const auto& stream = container->ShaderStream();
			const std::size_t first = instructions[decodeIndex].TokenOffset;
			const std::size_t last = instructions[decodeIndex + 5].TokenOffset + instructions[decodeIndex + 5].TokenLength;
			std::vector<std::uint32_t> duplicated;
			for (std::size_t i = first; i < last; ++i) {
				duplicated.push_back(GetU32(Bytes(stream.begin(), stream.end()), i * 4));
			}
			auto wrong = InsertRawTokens(original, first, duplicated);
			auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
			CHECK(inspected.has_value());
			auto expandedIdentity = identity;
			expandedIdentity.Identity.BytecodeSize = static_cast<std::uint32_t>(wrong.size());
			auto result = patcher.Patch(*inspected, expandedIdentity);
			CHECK(!result && result.error().Code == DxbcErrorCode::TargetRegionMissing);
		}
	}

	void MathTests()
	{
		auto decode = [](float x, float y, bool guard) {
			float f = x * x + y * y;
			if (guard && f > 4.0f) {
				const float scale = 1.999f / std::sqrt(f);
				x *= scale;
				y *= scale;
				f = x * x + y * y;
			}
			const float g = std::sqrt(1.0f - 0.25f * f);
			return std::array<float, 4>{ g * x, g * y, 0.5f * f - 1.0f, f };
		};
		for (const auto e : std::array<std::array<float, 2>, 4>{ { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.9f, 0.2f }, { 2.0f, 0.0f } } }) {
			const auto plain = decode(e[0], e[1], false);
			const auto guarded = decode(e[0], e[1], true);
			CHECK(plain == guarded);
		}
		const auto guarded = decode(-1.8346532434f, -1.0092011690f, true);
		CHECK(std::isfinite(guarded[0]) && std::isfinite(guarded[1]) && std::isfinite(guarded[2]));
		CHECK(guarded[3] < 4.0f);
		const float length = std::sqrt(guarded[0] * guarded[0] + guarded[1] * guarded[1] + guarded[2] * guarded[2]);
		CHECK(std::abs(length - 1.0f) < 0.00001f);
	}
}

auto main() -> int
{
	RealTargetTests();
	RejectionTests();
	MathTests();

	if (g_failures != 0) {
		std::cerr << "Core.DxbcLambertGuardPatcher: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Core.DxbcLambertGuardPatcher: all checks passed (10 real shaders, rejection matrix, math vectors).\n";
	return EXIT_SUCCESS;
}
