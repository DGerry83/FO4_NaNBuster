#include "Core/DxbcAnisotropicSpecularClampPatcher.h"

#include "Core/DxbcInspector.h"
#include "Core/DxbcLambertGuardPatcher.h"
#include "Core/ShaderIdentityMatcher.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace NaNBuster
{
	namespace
	{
		constexpr std::size_t kHashOffset = 4;
		constexpr std::size_t kDeclaredTotalOffset = 24;
		constexpr std::size_t kChunkTableOffset = 32;
		constexpr std::size_t kChunkHeaderSize = 8;

		constexpr std::uint32_t kShexFourCC = 0x58454853u;
		constexpr std::uint32_t kShdrFourCC = 0x52444853u;
		constexpr std::uint32_t kPixelShader50 = 0x00000050u;
		constexpr std::uint32_t kComputeShader50 = 0x00050050u;

		constexpr std::uint32_t kOpcodeExp = 25;
		constexpr std::uint32_t kOpcodeLog = 47;
		constexpr std::uint32_t kOpcodeMax = 52;
		constexpr std::uint32_t kOpcodeMin = 51;
		constexpr std::uint32_t kOpcodeMul = 56;
		constexpr std::uint32_t kOpcodeDclTemps = 104;

		constexpr std::uint32_t kFloatZero = 0;
		constexpr std::uint32_t kFloatOne = 0x3F800000u;
		constexpr std::uint32_t kImmediate32Operand = 0x00004001u;
		constexpr std::uint32_t kMinInstructionToken = 0x07000033u;

		constexpr std::size_t kInsertedTokenCount = 7;
		constexpr std::size_t kInsertedByteCount = kInsertedTokenCount * sizeof(std::uint32_t);

		auto ReadU32(const std::byte* p) noexcept -> std::uint32_t
		{
			return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[0])) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[1])) << 8) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[2])) << 16) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[3])) << 24);
		}

		void WriteU32(std::byte* p, std::uint32_t value) noexcept
		{
			for (unsigned i = 0; i < 4; ++i) {
				p[i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
			}
		}

		auto TokenAt(ByteView stream, std::size_t offset) noexcept -> std::uint32_t
		{
			return ReadU32(stream.data() + offset * sizeof(std::uint32_t));
		}

		auto FindShaderChunk(const DxbcContainerView& container) -> const DxbcChunkDescriptor*
		{
			for (const auto& chunk : container.Chunks()) {
				if (chunk.FourCC == kShexFourCC || chunk.FourCC == kShdrFourCC) {
					return &chunk;
				}
			}
			return nullptr;
		}

		struct ClampAnchor
		{
			std::size_t MaxTokenOffset;
			std::size_t InsertionTokenOffset;
		};

		auto FindClampAnchor(const DxbcContainerView& container) -> std::optional<ClampAnchor>
		{
			const auto instructions = container.Instructions();
			const ByteView stream = container.ShaderStream();
			std::optional<ClampAnchor> result;
			for (std::size_t i = 0; i + 4 < instructions.size(); ++i) {
				const auto& max = instructions[i];
				if (max.Opcode != kOpcodeMax || max.TokenLength != kInsertedTokenCount ||
					instructions[i + 1].Opcode != kOpcodeLog || instructions[i + 2].Opcode != kOpcodeMul ||
					instructions[i + 3].Opcode != kOpcodeExp || instructions[i + 4].Opcode != kOpcodeMul ||
					TokenAt(stream, max.TokenOffset + 2) != TokenAt(stream, max.TokenOffset + 4) ||
					TokenAt(stream, max.TokenOffset + 5) != kImmediate32Operand ||
					TokenAt(stream, max.TokenOffset + 6) != kFloatZero) {
					continue;
				}
				if (result) {
					return std::nullopt;
				}
				result = ClampAnchor{ max.TokenOffset, instructions[i + 1].TokenOffset };
			}
			return result;
		}

		bool HasClamp(const DxbcContainerView& container) noexcept
		{
			const auto instructions = container.Instructions();
			const ByteView stream = container.ShaderStream();
			for (std::size_t i = 0; i + 1 < instructions.size(); ++i) {
				const auto& min = instructions[i];
				if (min.Opcode == kOpcodeMin && min.TokenLength == kInsertedTokenCount &&
					instructions[i + 1].Opcode == kOpcodeLog &&
					TokenAt(stream, min.TokenOffset) == kMinInstructionToken &&
					TokenAt(stream, min.TokenOffset + 5) == kImmediate32Operand &&
					TokenAt(stream, min.TokenOffset + 6) == kFloatOne) {
					return true;
				}
			}
			return false;
		}

		struct TempsDeclaration
		{
			std::size_t TokenOffset;
			std::uint32_t Count;
		};

		auto FindTemps(const DxbcContainerView& container) -> std::optional<TempsDeclaration>
		{
			std::optional<TempsDeclaration> result;
			for (const auto& instruction : container.Instructions()) {
				if (instruction.Opcode != kOpcodeDclTemps) {
					continue;
				}
				if (result || instruction.TokenLength != 2) {
					return std::nullopt;
				}
				result = TempsDeclaration{
					instruction.TokenOffset,
					TokenAt(container.ShaderStream(), instruction.TokenOffset + 1),
				};
			}
			return result;
		}

		auto InsertClamp(const DxbcContainerView& container, const ClampAnchor& anchor)
			-> std::expected<std::vector<std::byte>, DxbcError>
		{
			const ByteView original = container.OriginalBytes();
			const ByteView stream = container.ShaderStream();
			const auto* shaderChunk = FindShaderChunk(container);
			if (shaderChunk == nullptr ||
				original.size() > std::numeric_limits<std::uint32_t>::max() - kInsertedByteCount) {
				return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, anchor.InsertionTokenOffset });
			}

			const std::size_t newTotal = original.size() + kInsertedByteCount;
			const std::size_t insertionFileOffset =
				container.ShaderStreamFileOffset() + anchor.InsertionTokenOffset * sizeof(std::uint32_t);
			const std::vector<std::uint32_t> tokens{
				kMinInstructionToken,
				TokenAt(stream, anchor.MaxTokenOffset + 1),
				TokenAt(stream, anchor.MaxTokenOffset + 2),
				TokenAt(stream, anchor.MaxTokenOffset + 3),
				TokenAt(stream, anchor.MaxTokenOffset + 4),
				kImmediate32Operand,
				kFloatOne,
			};

			std::vector<std::byte> corrected;
			corrected.reserve(newTotal);
			corrected.insert(corrected.end(), original.begin(), original.begin() + insertionFileOffset);
			for (const std::uint32_t token : tokens) {
				const std::size_t at = corrected.size();
				corrected.resize(at + sizeof(std::uint32_t));
				WriteU32(corrected.data() + at, token);
			}
			corrected.insert(corrected.end(), original.begin() + insertionFileOffset, original.end());

			WriteU32(corrected.data() + kDeclaredTotalOffset, static_cast<std::uint32_t>(newTotal));
			for (std::size_t i = 0; i < container.Chunks().size(); ++i) {
				const auto& chunk = container.Chunks()[i];
				if (chunk.Offset > shaderChunk->Offset) {
					const std::size_t entry = kChunkTableOffset + i * sizeof(std::uint32_t);
					WriteU32(corrected.data() + entry,
						static_cast<std::uint32_t>(ReadU32(corrected.data() + entry) + kInsertedByteCount));
				}
			}
			WriteU32(corrected.data() + shaderChunk->Offset + 4,
				static_cast<std::uint32_t>(shaderChunk->Size + kInsertedByteCount));
			WriteU32(corrected.data() + shaderChunk->Offset + kChunkHeaderSize + sizeof(std::uint32_t),
				static_cast<std::uint32_t>(container.ShaderTokenCount() + 2 + kInsertedTokenCount));
			return corrected;
		}
	}

	auto DxbcAnisotropicSpecularClampPatcher::Patch(
		const DxbcContainerView& container,
		const LambertShaderIdentity& identity,
		bool includeLambertGuard) const
		-> std::expected<DxbcAnisotropicSpecularClampPatchResult, DxbcError>
	try
	{
		const bool approvedTarget =
			identity.Identity.ShaderUid == LambertShaderIdentities[1].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[2].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[3].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[5].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[7].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[9].Identity.ShaderUid ||
			identity.Identity.ShaderUid == LambertShaderIdentities[11].Identity.ShaderUid;
		const bool computeTarget = identity.Identity.ShaderUid == LambertShaderIdentities[11].Identity.ShaderUid;
		const ByteView original = container.OriginalBytes();
		if (!approvedTarget) {
			return std::unexpected(DxbcError{ DxbcErrorCode::HashMismatch, kHashOffset });
		}
		if (original.size() != identity.Identity.BytecodeSize) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetSizeMismatch, kDeclaredTotalOffset });
		}
		if (!ShaderIdentityMatcher::Matches(original, identity.Identity)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::HashMismatch, kHashOffset });
		}

		const auto* shaderChunk = FindShaderChunk(container);
		if (shaderChunk == nullptr) {
			return std::unexpected(DxbcError{ DxbcErrorCode::MissingShaderChunk, 0 });
		}
		const auto expectedStage = computeTarget ? kComputeShader50 : kPixelShader50;
		const auto expectedIdentityStage = computeTarget ? LambertShaderStage::Compute : LambertShaderStage::Pixel;
		if (identity.Stage != expectedIdentityStage ||
			ReadU32(original.data() + shaderChunk->Offset + kChunkHeaderSize) != expectedStage) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, shaderChunk->Offset });
		}
		if (HasClamp(container)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::AlreadyPatched, 0 });
		}
		const auto originalTemps = FindTemps(container);
		const auto originalAnchor = FindClampAnchor(container);
		if (!originalTemps || !originalAnchor) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, 0 });
		}

		std::vector<std::byte> guarded;
		std::optional<DxbcContainerView> guardedContainerStorage;
		const DxbcContainerView* baseContainer = &container;
		if (includeLambertGuard) {
			auto guard = DxbcLambertGuardPatcher{}.Patch(container, identity);
			if (!guard) {
				return std::unexpected(guard.error());
			}
			guarded = std::move(guard->Bytes);
			auto inspected = DxbcInspector{}.Inspect(ByteView{ guarded.data(), guarded.size() });
			if (!inspected) {
				return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, inspected.error().Offset });
			}
			guardedContainerStorage = std::move(*inspected);
			baseContainer = &*guardedContainerStorage;
		}

		const auto anchor = FindClampAnchor(*baseContainer);
		const auto baseTemps = FindTemps(*baseContainer);
		if (!anchor || !baseTemps ||
			baseTemps->Count != originalTemps->Count + (includeLambertGuard ? 2u : 0u) ||
			baseContainer->Instructions().size() != container.Instructions().size() + (includeLambertGuard ? 6u : 0u)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
		}
		auto corrected = InsertClamp(*baseContainer, *anchor);
		if (!corrected) {
			return std::unexpected(corrected.error());
		}

		auto recheck = DxbcInspector{}.Inspect(ByteView{ corrected->data(), corrected->size() });
		if (!recheck) {
			return std::unexpected(recheck.error());
		}
		const auto newTemps = FindTemps(*recheck);
		if (recheck->Instructions().size() != baseContainer->Instructions().size() + 1 ||
			recheck->ShaderTokenCount() != baseContainer->ShaderTokenCount() + kInsertedTokenCount ||
			!newTemps || newTemps->Count != baseTemps->Count || !HasClamp(*recheck)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, anchor->InsertionTokenOffset });
		}

		return DxbcAnisotropicSpecularClampPatchResult{
			.Bytes = std::move(*corrected),
			.InsertionTokenOffset = anchor->InsertionTokenOffset,
			.InsertionFileOffset = baseContainer->ShaderStreamFileOffset() + anchor->InsertionTokenOffset * sizeof(std::uint32_t),
			.InsertedTokenCount = kInsertedTokenCount,
			.OriginalTempCount = originalTemps->Count,
			.IncludesLambertGuard = includeLambertGuard,
		};
	}
	catch (...) {
		return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
	}
}
