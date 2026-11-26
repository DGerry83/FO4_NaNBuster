#include "Core/DxbcLambertGuardPatcher.h"

#include "Core/DxbcInspector.h"
#include "Core/ShaderIdentityMatcher.h"

#include <array>
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

		constexpr std::uint32_t kOpcodeDp2 = 15;
		constexpr std::uint32_t kOpcodeLt = 49;
		constexpr std::uint32_t kOpcodeMad = 50;
		constexpr std::uint32_t kOpcodeMov = 54;
		constexpr std::uint32_t kOpcodeMovc = 55;
		constexpr std::uint32_t kOpcodeMul = 56;
		constexpr std::uint32_t kOpcodeRsq = 68;
		constexpr std::uint32_t kOpcodeSqrt = 75;
		constexpr std::uint32_t kOpcodeDclTemps = 104;

		constexpr std::uint32_t kFloatOne = 0x3F800000u;
		constexpr std::uint32_t kFloatTwo = 0x40000000u;
		constexpr std::uint32_t kFloatFour = 0x40800000u;
		constexpr std::uint32_t kFloatOne999 = 0x3FFFDF3Bu;
		constexpr std::uint32_t kFloatQuarter = 0x3E800000u;
		constexpr std::uint32_t kFloatHalf = 0x3F000000u;
		constexpr std::uint32_t kFloatNegativeTwo = 0xC0000000u;
		constexpr std::uint32_t kImmediate32Operand = 0x00004001u;

		constexpr std::size_t kInsertedTokenCount = 42;
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

		bool ContainsToken(ByteView stream, const DxbcInstructionDescriptor& instruction, std::uint32_t value) noexcept
		{
			for (std::size_t i = 0; i < instruction.TokenLength; ++i) {
				if (TokenAt(stream, instruction.TokenOffset + i) == value) {
					return true;
				}
			}
			return false;
		}

		bool HasGuard(const DxbcContainerView& container) noexcept
		{
			const auto instructions = container.Instructions();
			for (std::size_t i = 0; i + 5 < instructions.size(); ++i) {
				if (instructions[i].Opcode == kOpcodeLt &&
					instructions[i + 1].Opcode == kOpcodeRsq &&
					instructions[i + 2].Opcode == kOpcodeMul &&
					instructions[i + 3].Opcode == kOpcodeMovc &&
					instructions[i + 4].Opcode == kOpcodeMul &&
					instructions[i + 5].Opcode == kOpcodeDp2 &&
					ContainsToken(container.ShaderStream(), instructions[i + 2], kFloatOne999)) {
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

		struct DecodeAnchor
		{
			std::size_t MadTokenOffset;
			std::size_t Dp2TokenOffset;
			std::size_t InsertionTokenOffset;
			std::uint32_t FRegister;
			std::uint32_t FComponent;
		};

		auto FindDecodeAnchor(const DxbcContainerView& container) -> std::optional<DecodeAnchor>
		{
			const auto instructions = container.Instructions();
			const ByteView stream = container.ShaderStream();
			std::optional<DecodeAnchor> result;
			for (std::size_t i = 0; i + 5 < instructions.size(); ++i) {
				const auto& mad = instructions[i];
				const auto& dp2 = instructions[i + 1];
				const auto& coefficients = instructions[i + 2];
				if (mad.Opcode != kOpcodeMad || dp2.Opcode != kOpcodeDp2 ||
					coefficients.Opcode != kOpcodeMad || instructions[i + 3].Opcode != kOpcodeSqrt ||
					instructions[i + 4].Opcode != kOpcodeMul ||
					(instructions[i + 5].Opcode != kOpcodeMov && instructions[i + 5].Opcode != kOpcodeMul) ||
					dp2.TokenLength != 7) {
					continue;
				}

				const std::uint32_t eToken = TokenAt(stream, dp2.TokenOffset + 3);
				const std::uint32_t eRegister = TokenAt(stream, dp2.TokenOffset + 4);
				if (eToken != TokenAt(stream, dp2.TokenOffset + 5) ||
					eRegister != TokenAt(stream, dp2.TokenOffset + 6) ||
					eRegister != TokenAt(stream, mad.TokenOffset + 2) ||
					!ContainsToken(stream, mad, kFloatFour) ||
					!ContainsToken(stream, mad, kFloatNegativeTwo) ||
					!ContainsToken(stream, coefficients, kFloatQuarter) ||
					!ContainsToken(stream, coefficients, kFloatHalf) ||
					!ContainsToken(stream, coefficients, kFloatOne)) {
					continue;
				}

				const std::uint32_t fToken = TokenAt(stream, dp2.TokenOffset + 1);
				const std::uint32_t fRegister = TokenAt(stream, dp2.TokenOffset + 2);
				const std::uint32_t fMask = (fToken >> 4) & 0x0Fu;
				if ((fToken & 0x0000000Cu) != 0 || fMask == 0 || (fMask & (fMask - 1)) != 0 ||
					!ContainsToken(stream, coefficients, fRegister)) {
					continue;
				}
				if (result) {
					return std::nullopt;
				}
				const std::uint32_t fComponent = fMask == 1 ? 0 : fMask == 2 ? 1 : fMask == 4 ? 2 : 3;
				result = DecodeAnchor{
					mad.TokenOffset,
					dp2.TokenOffset,
					coefficients.TokenOffset,
					fRegister,
					fComponent,
				};
			}
			return result;
		}

		auto TempDestination(std::uint32_t index) -> std::pair<std::uint32_t, std::uint32_t>
		{
			return { 0x00100012u, index };
		}

		auto TempSource(std::uint32_t component, std::uint32_t index) -> std::pair<std::uint32_t, std::uint32_t>
		{
			return { 0x00100006u | ((component * 0x55u) << 4), index };
		}

		void AppendOperand(std::vector<std::uint32_t>& tokens, std::pair<std::uint32_t, std::uint32_t> operand)
		{
			tokens.push_back(operand.first);
			tokens.push_back(operand.second);
		}

		void AppendImmediate(std::vector<std::uint32_t>& tokens, std::uint32_t value)
		{
			tokens.push_back(kImmediate32Operand);
			tokens.push_back(value);
		}

		auto BuildGuardTokens(ByteView stream, const DecodeAnchor& anchor, std::uint32_t originalTempCount, bool useMadSource)
			-> std::vector<std::uint32_t>
		{
			const auto mask = TempDestination(originalTempCount);
			const auto scale = TempDestination(originalTempCount + 1);
			const auto maskSource = TempSource(0, originalTempCount);
			const auto scaleSource = TempSource(0, originalTempCount + 1);
			const auto fSource = TempSource(anchor.FComponent, anchor.FRegister);
			const std::pair<std::uint32_t, std::uint32_t> eDestination{
				TokenAt(stream, anchor.MadTokenOffset + 1),
				TokenAt(stream, anchor.MadTokenOffset + 2),
			};
			const std::size_t eSourceOffset = useMadSource ? anchor.MadTokenOffset + 3 : anchor.Dp2TokenOffset + 3;
			const std::pair<std::uint32_t, std::uint32_t> eSource{
				TokenAt(stream, eSourceOffset),
				TokenAt(stream, eSourceOffset + 1),
			};
			const std::pair<std::uint32_t, std::uint32_t> eDotSource{
				TokenAt(stream, anchor.Dp2TokenOffset + 3),
				TokenAt(stream, anchor.Dp2TokenOffset + 4),
			};
			const std::pair<std::uint32_t, std::uint32_t> fDestination{
				TokenAt(stream, anchor.Dp2TokenOffset + 1),
				TokenAt(stream, anchor.Dp2TokenOffset + 2),
			};

			std::vector<std::uint32_t> tokens;
			tokens.reserve(kInsertedTokenCount);
			tokens.push_back(0x07000031u);
			AppendOperand(tokens, mask);
			AppendImmediate(tokens, kFloatFour);
			AppendOperand(tokens, fSource);

			tokens.push_back(0x05000044u);
			AppendOperand(tokens, scale);
			AppendOperand(tokens, fSource);

			tokens.push_back(0x07000038u);
			AppendOperand(tokens, scale);
			AppendOperand(tokens, scaleSource);
			AppendImmediate(tokens, kFloatOne999);

			tokens.push_back(0x09000037u);
			AppendOperand(tokens, scale);
			AppendOperand(tokens, maskSource);
			AppendOperand(tokens, scaleSource);
			AppendImmediate(tokens, kFloatOne);

			tokens.push_back(0x07000038u);
			AppendOperand(tokens, eDestination);
			AppendOperand(tokens, eSource);
			AppendOperand(tokens, scaleSource);

			tokens.push_back(0x0700000Fu);
			AppendOperand(tokens, fDestination);
			AppendOperand(tokens, eDotSource);
			AppendOperand(tokens, eDotSource);
			return tokens;
		}
	}

	auto DxbcLambertGuardPatcher::Patch(const DxbcContainerView& container, const LambertShaderIdentity& identity) const
		-> std::expected<DxbcLambertGuardPatchResult, DxbcError>
	try
	{
		const ByteView original = container.OriginalBytes();
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
		const std::uint32_t expectedStage = identity.Stage == LambertShaderStage::Compute ? kComputeShader50 : kPixelShader50;
		if (ReadU32(original.data() + shaderChunk->Offset + kChunkHeaderSize) != expectedStage) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, shaderChunk->Offset });
		}
		if (HasGuard(container)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::AlreadyPatched, 0 });
		}

		const auto temps = FindTemps(container);
		if (!temps || temps->Count > 4094) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, 0 });
		}
		const auto anchor = FindDecodeAnchor(container);
		if (!anchor) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, 0 });
		}

		const ByteView stream = container.ShaderStream();
		const bool useMadSource = identity.Identity.ShaderUid == LambertShaderIdentities[11].Identity.ShaderUid;
		const auto guardTokens = BuildGuardTokens(stream, *anchor, temps->Count, useMadSource);
		if (guardTokens.size() != kInsertedTokenCount ||
			original.size() > std::numeric_limits<std::uint32_t>::max() - kInsertedByteCount) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, anchor->InsertionTokenOffset });
		}

		const std::size_t newTotal = original.size() + kInsertedByteCount;
		const std::size_t insertionFileOffset =
			container.ShaderStreamFileOffset() + anchor->InsertionTokenOffset * sizeof(std::uint32_t);
		std::vector<std::byte> corrected;
		corrected.reserve(newTotal);
		corrected.insert(corrected.end(), original.begin(), original.begin() + insertionFileOffset);
		for (const std::uint32_t token : guardTokens) {
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
		WriteU32(corrected.data() + container.ShaderStreamFileOffset() + temps->TokenOffset * sizeof(std::uint32_t) + 4,
			temps->Count + 2);

		const DxbcInspector inspector;
		auto recheck = inspector.Inspect(ByteView{ corrected.data(), corrected.size() });
		if (!recheck.has_value()) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, recheck.error().Offset });
		}
		const auto newTemps = FindTemps(*recheck);
		if (recheck->Instructions().size() != container.Instructions().size() + 6 ||
			recheck->ShaderTokenCount() != container.ShaderTokenCount() + kInsertedTokenCount ||
			!newTemps || newTemps->Count != temps->Count + 2 || !HasGuard(*recheck)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, anchor->InsertionTokenOffset });
		}

		return DxbcLambertGuardPatchResult{
			.Bytes = std::move(corrected),
			.InsertionTokenOffset = anchor->InsertionTokenOffset,
			.InsertionFileOffset = insertionFileOffset,
			.InsertedTokenCount = kInsertedTokenCount,
			.OriginalTempCount = temps->Count,
		};
	}
	catch (...) {
		return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
	}
}
