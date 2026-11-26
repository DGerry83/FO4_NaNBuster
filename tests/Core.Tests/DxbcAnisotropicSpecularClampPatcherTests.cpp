#include "Core/DxbcAnisotropicSpecularClampPatcher.h"
#include "Core/DxbcInspector.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
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

	constexpr std::size_t kClampBytes = 28;
	constexpr std::size_t kGuardBytes = 168;
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

	auto Token(const DxbcContainerView& container, std::size_t offset) -> std::uint32_t
	{
		Bytes stream(container.ShaderStream().begin(), container.ShaderStream().end());
		return GetU32(stream, offset * 4);
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

	auto Temps(const DxbcContainerView& container) -> std::uint32_t
	{
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode == 104) {
				return Token(container, instruction.TokenOffset + 1);
			}
		}
		return 0;
	}

	auto FindClampAnchor(const DxbcContainerView& container) -> std::size_t
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 4 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 52 && instructions[i + 1].Opcode == 47 &&
				instructions[i + 2].Opcode == 56 && instructions[i + 3].Opcode == 25 &&
				instructions[i + 4].Opcode == 56 && Token(container, instructions[i].TokenOffset + 6) == 0) {
				return i;
			}
		}
		return instructions.size();
	}

	bool HasClamp(const DxbcContainerView& container)
	{
		const auto instructions = container.Instructions();
		for (std::size_t i = 0; i + 1 < instructions.size(); ++i) {
			if (instructions[i].Opcode == 51 && instructions[i + 1].Opcode == 47 &&
				Token(container, instructions[i].TokenOffset + 6) == 0x3F800000u) {
				return true;
			}
		}
		return false;
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

	void CheckRemainder(
		const Bytes& original,
		const Bytes& corrected,
		const DxbcContainerView& originalContainer,
		std::size_t insertionFile)
	{
		const auto& shader = ShaderChunk(originalContainer);
		auto allowed = [&](std::size_t offset) {
			if ((offset >= 24 && offset < 28) || (offset >= shader.Offset + 4 && offset < shader.Offset + 16)) {
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
				CHECK(corrected[i + kClampBytes] == original[i]);
			}
		}
	}

	void CheckClampTokens(
		const DxbcContainerView& originalContainer,
		const Bytes& corrected,
		std::size_t anchorIndex,
		std::size_t insertionFile)
	{
		const auto& max = originalContainer.Instructions()[anchorIndex];
		const std::array<std::uint32_t, 7> expected{
			0x07000033u,
			Token(originalContainer, max.TokenOffset + 1),
			Token(originalContainer, max.TokenOffset + 2),
			Token(originalContainer, max.TokenOffset + 3),
			Token(originalContainer, max.TokenOffset + 4),
			0x00004001u,
			0x3F800000u,
		};
		for (std::size_t i = 0; i < expected.size(); ++i) {
			CHECK(GetU32(corrected, insertionFile + i * 4) == expected[i]);
		}
	}
}

int RunTarget(const NaNBuster::LambertShaderIdentity& identity)
{
	const std::filesystem::path path = std::filesystem::path(NANBUSTER_LAMBERT_REFERENCE_ROOT) /
		identity.ReferenceStem / "original.dxbc";
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile(path.string());
	auto container = Inspect(NaNBuster::TestFixtures::AsByteView(original));
	CHECK(container.has_value());
	if (!container) {
		return EXIT_FAILURE;
	}
	const std::size_t anchorIndex = FindClampAnchor(*container);
	CHECK(anchorIndex != container->Instructions().size());
	CHECK(!HasClamp(*container));
	CHECK(!HasGuard(*container));

	NaNBuster::DxbcAnisotropicSpecularClampPatcher patcher;
	auto clamp = patcher.Patch(*container, identity);
	CHECK(clamp.has_value());
	if (!clamp) {
		std::cerr << "clamp error code=" << static_cast<int>(clamp.error().Code) << " offset=" << clamp.error().Offset << '\n';
	}
	if (clamp) {
		CHECK(clamp->Bytes.size() == original.size() + kClampBytes);
		CHECK(clamp->InsertedTokenCount == 7);
		CHECK(clamp->OriginalTempCount == Temps(*container));
		CHECK(!clamp->IncludesLambertGuard);
		CHECK(clamp->InsertionTokenOffset == container->Instructions()[anchorIndex + 1].TokenOffset);
		auto corrected = Inspect(NaNBuster::TestFixtures::AsByteView(clamp->Bytes));
		CHECK(corrected.has_value());
		if (corrected) {
			CHECK(corrected->Instructions().size() == container->Instructions().size() + 1);
			CHECK(corrected->ShaderTokenCount() == container->ShaderTokenCount() + 7);
			CHECK(Temps(*corrected) == Temps(*container));
			CHECK(HasClamp(*corrected));
			CHECK(!HasGuard(*corrected));
		}
		CheckClampTokens(*container, clamp->Bytes, anchorIndex, clamp->InsertionFileOffset);
		CheckRemainder(original, clamp->Bytes, *container, clamp->InsertionFileOffset);

		auto repeat = patcher.Patch(*container, identity);
		CHECK(repeat.has_value() && repeat->Bytes == clamp->Bytes);
		auto correctedIdentity = identity;
		correctedIdentity.Identity.BytecodeSize = static_cast<std::uint32_t>(clamp->Bytes.size());
		auto correctedContainer = Inspect(NaNBuster::TestFixtures::AsByteView(clamp->Bytes));
		auto already = patcher.Patch(*correctedContainer, correctedIdentity);
		CHECK(!already && already.error().Code == DxbcErrorCode::AlreadyPatched);
	}

	auto guarded = NaNBuster::DxbcLambertGuardPatcher{}.Patch(*container, identity);
	CHECK(guarded.has_value());
	auto combined = patcher.Patch(*container, identity, true);
	CHECK(combined.has_value());
	if (!combined) {
		std::cerr << "combined error code=" << static_cast<int>(combined.error().Code) << " offset=" << combined.error().Offset << '\n';
	}
	if (guarded && combined) {
		CHECK(combined->Bytes.size() == original.size() + kGuardBytes + kClampBytes);
		CHECK(combined->IncludesLambertGuard);
		CHECK(combined->OriginalTempCount == Temps(*container));
		auto guardedContainer = Inspect(NaNBuster::TestFixtures::AsByteView(guarded->Bytes));
		auto combinedContainer = Inspect(NaNBuster::TestFixtures::AsByteView(combined->Bytes));
		CHECK(guardedContainer.has_value());
		CHECK(combinedContainer.has_value());
		if (guardedContainer && combinedContainer) {
			CHECK(combinedContainer->Instructions().size() == container->Instructions().size() + 7);
			CHECK(combinedContainer->ShaderTokenCount() == container->ShaderTokenCount() + 49);
			CHECK(Temps(*combinedContainer) == Temps(*container) + 2);
			CHECK(HasClamp(*combinedContainer));
			CHECK(HasGuard(*combinedContainer));
			const std::size_t guardedAnchor = FindClampAnchor(*guardedContainer);
			CHECK(guardedAnchor != guardedContainer->Instructions().size());
			CheckClampTokens(*guardedContainer, combined->Bytes, guardedAnchor, combined->InsertionFileOffset);
			CheckRemainder(guarded->Bytes, combined->Bytes, *guardedContainer, combined->InsertionFileOffset);
		}
	}

	auto other = patcher.Patch(*container, NaNBuster::LambertShaderIdentities[0]);
	CHECK(!other && other.error().Code == DxbcErrorCode::HashMismatch);
	{
		auto wrong = original;
		wrong[4] ^= std::byte{ 0xFF };
		auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
		auto result = patcher.Patch(*inspected, identity);
		CHECK(!result && result.error().Code == DxbcErrorCode::HashMismatch);
	}
	{
		auto wrong = original;
		const auto& max = container->Instructions()[anchorIndex];
		SetU32(wrong, container->ShaderStreamFileOffset() + (max.TokenOffset + 6) * 4, 0x40000000u);
		auto inspected = Inspect(NaNBuster::TestFixtures::AsByteView(wrong));
		auto result = patcher.Patch(*inspected, identity);
		CHECK(!result && result.error().Code == DxbcErrorCode::TargetRegionMissing);
	}

	for (const float base : std::array{ 0.0f, 0.25f, 0.99f, 1.0f }) {
		CHECK(std::pow(std::min(base, 1.0f), 125.0f) == std::pow(base, 125.0f));
	}
	CHECK(std::pow(std::min(1.1175971031f, 1.0f), 125.0f) == 1.0f);
	CHECK(std::pow(std::min(1.0708956718f, 1.0f), 125.0f) == 1.0f);
	CHECK(13088.9267578125 / 1085570.625 > 0.01205718);
	CHECK(13088.9267578125 / 1085570.625 < 0.01205719);

	if (g_failures != 0) {
		std::cerr << "Core.DxbcAnisotropicSpecularClampPatcher: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Core.DxbcAnisotropicSpecularClampPatcher: all checks passed for " << identity.Identity.ShaderUid
			  << " (clamp-only, combined, remainder, rejection, math).\n";
	return EXIT_SUCCESS;
}

auto main() -> int
{
	for (const auto index : { 1, 2, 3, 5, 7, 9, 11 }) {
		if (RunTarget(NaNBuster::LambertShaderIdentities[index]) != EXIT_SUCCESS) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
