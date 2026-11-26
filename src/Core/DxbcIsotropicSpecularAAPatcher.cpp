#include "Core/DxbcIsotropicSpecularAAPatcher.h"

#include "Core/DxbcInspector.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "Core/ShaderIdentityMatcher.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
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

		constexpr std::uint32_t kOpcodeAdd = 0;
		constexpr std::uint32_t kOpcodeDiv = 14;
		constexpr std::uint32_t kOpcodeDp3 = 16;
		constexpr std::uint32_t kOpcodeMin = 51;
		constexpr std::uint32_t kOpcodeMul = 56;
		constexpr std::uint32_t kOpcodeDerivRtxFine = 123;
		constexpr std::uint32_t kOpcodeDerivRtyFine = 125;
		constexpr std::uint32_t kOpcodeDclTemps = 104;
		constexpr std::uint32_t kInstructionSaturate = 0x00002000u;

		constexpr std::uint32_t kFloatTwo = 0x40000000u;
		constexpr std::uint32_t kFloatQuarter = 0x3E800000u;
		constexpr std::uint32_t kFloatVarianceCap = 0x3CA3D70Au;
		constexpr std::uint32_t kFloatNegativeTwo = 0xC0000000u;
		constexpr std::uint32_t kImmediate32Operand = 0x00004001u;

		constexpr std::size_t kCullTokenOffset = 370;
		constexpr std::size_t kDecodeTokenOffset = 403;
		constexpr std::size_t kExponentMadTokenOffset = 1014;
		constexpr std::size_t kExponentTokenOffset = 1023;
		constexpr std::size_t kExponentInsertionTokenOffset = 1028;
		constexpr std::size_t kVarianceTokenCount = 101;
		constexpr std::size_t kExponentTokenCount = 35;
		constexpr std::size_t kInsertedTokenCount = kVarianceTokenCount + kExponentTokenCount;
		constexpr std::size_t kInsertedByteCount = kInsertedTokenCount * sizeof(std::uint32_t);
		constexpr std::size_t kOriginalTempCount = 9;

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

		bool HasInstruction(
			const DxbcContainerView& container,
			std::size_t tokenOffset,
			std::uint32_t opcode,
			std::size_t tokenLength) noexcept
		{
			for (const auto& instruction : container.Instructions()) {
				if (instruction.TokenOffset == tokenOffset) {
					return instruction.Opcode == opcode && instruction.TokenLength == tokenLength;
				}
			}
			return false;
		}

		bool HasRequiredAnchors(const DxbcContainerView& container) noexcept
		{
			return container.ShaderTokenCount() == 1585 &&
				HasInstruction(container, 370, 31, 3) &&
				HasInstruction(container, 373, 54, 8) &&
				HasInstruction(container, 381, 54, 8) &&
				HasInstruction(container, 389, 62, 1) &&
				HasInstruction(container, 390, 21, 1) &&
				HasInstruction(container, 391, 68, 5) &&
				HasInstruction(container, 403, 50, 15) &&
				HasInstruction(container, 418, 15, 7) &&
				HasInstruction(container, 425, 50, 16) &&
				HasInstruction(container, 441, 75, 5) &&
				HasInstruction(container, 446, 56, 7) &&
				HasInstruction(container, 453, 54, 6) &&
				HasInstruction(container, kExponentMadTokenOffset, 50, 9) &&
				HasInstruction(container, kExponentTokenOffset, 25, 5) &&
				HasInstruction(container, kExponentInsertionTokenOffset, kOpcodeDp3, 7);
		}

		auto FindTemps(const DxbcContainerView& container) -> std::optional<std::uint32_t>
		{
			std::optional<std::uint32_t> result;
			for (const auto& instruction : container.Instructions()) {
				if (instruction.Opcode != kOpcodeDclTemps) {
					continue;
				}
				if (result || instruction.TokenLength != 2) {
					return std::nullopt;
				}
				result = TokenAt(container.ShaderStream(), instruction.TokenOffset + 1);
			}
			return result;
		}

		bool HasSpecularAA(const DxbcContainerView& container) noexcept
		{
			bool fineX = false;
			bool fineY = false;
			bool cap = false;
			for (const auto& instruction : container.Instructions()) {
				fineX = fineX || instruction.Opcode == kOpcodeDerivRtxFine;
				fineY = fineY || instruction.Opcode == kOpcodeDerivRtyFine;
				if (instruction.Opcode == kOpcodeMin) {
					for (std::size_t i = 0; i < instruction.TokenLength; ++i) {
						cap = cap || TokenAt(container.ShaderStream(), instruction.TokenOffset + i) == kFloatVarianceCap;
					}
				}
			}
			return fineX && fineY && cap;
		}

		struct TokenEdit
		{
			std::size_t RelativeOffset;
			std::uint32_t Value;
		};

		void AppendCloned(
			std::vector<std::uint32_t>& tokens,
			ByteView stream,
			std::size_t tokenOffset,
			std::size_t tokenLength,
			std::initializer_list<TokenEdit> edits)
		{
			const std::size_t base = tokens.size();
			for (std::size_t i = 0; i < tokenLength; ++i) {
				tokens.push_back(TokenAt(stream, tokenOffset + i));
			}
			for (const auto& edit : edits) {
				tokens[base + edit.RelativeOffset] = edit.Value;
			}
		}

		auto TempDestination(std::uint32_t index, std::uint32_t mask) -> std::pair<std::uint32_t, std::uint32_t>
		{
			return { 0x00100002u | (mask << 4), index };
		}

		auto TempSource(std::uint32_t index, std::uint32_t component) -> std::pair<std::uint32_t, std::uint32_t>
		{
			return { 0x0010000Au | (component << 4), index };
		}

		auto TempSourceXyz(std::uint32_t index) -> std::pair<std::uint32_t, std::uint32_t>
		{
			return { 0x00100246u, index };
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

		void AppendUnary(
			std::vector<std::uint32_t>& tokens,
			std::uint32_t instructionToken,
			std::pair<std::uint32_t, std::uint32_t> destination,
			std::pair<std::uint32_t, std::uint32_t> source)
		{
			tokens.push_back(instructionToken);
			AppendOperand(tokens, destination);
			AppendOperand(tokens, source);
		}

		void AppendBinary(
			std::vector<std::uint32_t>& tokens,
			std::uint32_t instructionToken,
			std::pair<std::uint32_t, std::uint32_t> destination,
			std::pair<std::uint32_t, std::uint32_t> source0,
			std::pair<std::uint32_t, std::uint32_t> source1)
		{
			tokens.push_back(instructionToken);
			AppendOperand(tokens, destination);
			AppendOperand(tokens, source0);
			AppendOperand(tokens, source1);
		}

		void AppendBinaryImmediate(
			std::vector<std::uint32_t>& tokens,
			std::uint32_t instructionToken,
			std::pair<std::uint32_t, std::uint32_t> destination,
			std::pair<std::uint32_t, std::uint32_t> source,
			std::uint32_t immediate)
		{
			tokens.push_back(instructionToken);
			AppendOperand(tokens, destination);
			AppendOperand(tokens, source);
			AppendImmediate(tokens, immediate);
		}

		void AppendImmediateBinary(
			std::vector<std::uint32_t>& tokens,
			std::uint32_t instructionToken,
			std::pair<std::uint32_t, std::uint32_t> destination,
			std::uint32_t immediate,
			std::pair<std::uint32_t, std::uint32_t> source)
		{
			tokens.push_back(instructionToken);
			AppendOperand(tokens, destination);
			AppendImmediate(tokens, immediate);
			AppendOperand(tokens, source);
		}

		auto BuildVarianceTokens(ByteView stream) -> std::vector<std::uint32_t>
		{
			std::vector<std::uint32_t> tokens;
			tokens.reserve(kVarianceTokenCount);
			AppendCloned(tokens, stream, 403, 15, { { 2, 8 } });
			AppendCloned(tokens, stream, 418, 7, { { 2, 4 }, { 4, 8 }, { 6, 8 } });
			AppendCloned(tokens, stream, 425, 16, { { 5, 4 } });
			AppendCloned(tokens, stream, 441, 5, { { 1, 0x00100022u }, { 2, 4 } });
			AppendCloned(tokens, stream, 446, 7, { { 2, 8 }, { 3, 0x00100556u }, { 4, 4 }, { 6, 8 } });
			AppendCloned(tokens, stream, 453, 6, { { 2, 8 } });

			AppendUnary(tokens, 0x05000000u | kOpcodeDerivRtxFine, TempDestination(5, 0x7), TempSourceXyz(8));
			AppendUnary(tokens, 0x05000000u | kOpcodeDerivRtyFine, TempDestination(6, 0x7), TempSourceXyz(8));
			AppendBinary(tokens, 0x07000000u | kOpcodeDp3, TempDestination(5, 0x8), TempSourceXyz(5), TempSourceXyz(5));
			AppendBinary(tokens, 0x07000000u | kOpcodeDp3, TempDestination(6, 0x8), TempSourceXyz(6), TempSourceXyz(6));
			AppendBinary(tokens, 0x07000000u | kOpcodeAdd, TempDestination(5, 0x8), TempSource(5, 3), TempSource(6, 3));
			AppendBinaryImmediate(tokens, 0x07000000u | kOpcodeMul, TempDestination(5, 0x8), TempSource(5, 3), kFloatQuarter);
			AppendBinaryImmediate(tokens, 0x07000000u | kOpcodeMin, TempDestination(8, 0x8), TempSource(5, 3), kFloatVarianceCap);
			return tokens;
		}

		auto BuildExponentTokens() -> std::vector<std::uint32_t>
		{
			std::vector<std::uint32_t> tokens;
			tokens.reserve(kExponentTokenCount);
			AppendBinaryImmediate(tokens, 0x07000000u | kOpcodeAdd, TempDestination(6, 0x1), TempSource(3, 0), kFloatTwo);
			AppendImmediateBinary(tokens, 0x07000000u | kOpcodeDiv, TempDestination(6, 0x1), kFloatTwo, TempSource(6, 0));
			AppendBinary(
				tokens,
				0x07000000u | kInstructionSaturate | kOpcodeAdd,
				TempDestination(6, 0x1),
				TempSource(6, 0),
				TempSource(8, 3));
			AppendImmediateBinary(tokens, 0x07000000u | kOpcodeDiv, TempDestination(6, 0x1), kFloatTwo, TempSource(6, 0));
			AppendBinaryImmediate(tokens, 0x07000000u | kOpcodeAdd, TempDestination(3, 0x1), TempSource(6, 0), kFloatNegativeTwo);
			return tokens;
		}

		auto InsertTokens(
			const DxbcContainerView& container,
			std::size_t insertionTokenOffset,
			const std::vector<std::uint32_t>& tokens)
			-> std::expected<std::vector<std::byte>, DxbcError>
		{
			const ByteView original = container.OriginalBytes();
			const auto* shaderChunk = FindShaderChunk(container);
			const std::size_t insertedBytes = tokens.size() * sizeof(std::uint32_t);
			if (shaderChunk == nullptr || original.size() > std::numeric_limits<std::uint32_t>::max() - insertedBytes) {
				return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, insertionTokenOffset });
			}

			const std::size_t newTotal = original.size() + insertedBytes;
			const std::size_t insertionFileOffset =
				container.ShaderStreamFileOffset() + insertionTokenOffset * sizeof(std::uint32_t);
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
						static_cast<std::uint32_t>(ReadU32(corrected.data() + entry) + insertedBytes));
				}
			}
			WriteU32(corrected.data() + shaderChunk->Offset + 4,
				static_cast<std::uint32_t>(shaderChunk->Size + insertedBytes));
			WriteU32(corrected.data() + shaderChunk->Offset + kChunkHeaderSize + sizeof(std::uint32_t),
				static_cast<std::uint32_t>(container.ShaderTokenCount() + 2 + tokens.size()));
			return corrected;
		}
	}

	auto DxbcIsotropicSpecularAAPatcher::Patch(const DxbcContainerView& container) const
		-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>
	try
	{
		const ByteView original = container.OriginalBytes();
		if (HasSpecularAA(container)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::AlreadyPatched, 0 });
		}
		if (original.size() != IsotropicSpecularTargetIdentity.BytecodeSize) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetSizeMismatch, kDeclaredTotalOffset });
		}
		if (!ShaderIdentityMatcher::Matches(original, IsotropicSpecularTargetIdentity)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::HashMismatch, kHashOffset });
		}

		const auto* shaderChunk = FindShaderChunk(container);
		if (shaderChunk == nullptr) {
			return std::unexpected(DxbcError{ DxbcErrorCode::MissingShaderChunk, 0 });
		}
		if (ReadU32(original.data() + shaderChunk->Offset + kChunkHeaderSize) != kPixelShader50 ||
			shaderChunk->Offset + kChunkHeaderSize + shaderChunk->Size != original.size()) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, shaderChunk->Offset });
		}
		const auto temps = FindTemps(container);
		if (!temps || *temps != kOriginalTempCount || !HasRequiredAnchors(container)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, 0 });
		}

		const auto varianceTokens = BuildVarianceTokens(container.ShaderStream());
		const auto exponentTokens = BuildExponentTokens();
		if (varianceTokens.size() != kVarianceTokenCount || exponentTokens.size() != kExponentTokenCount) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, kCullTokenOffset });
		}

		auto withVariance = InsertTokens(container, kCullTokenOffset, varianceTokens);
		if (!withVariance) {
			return std::unexpected(withVariance.error());
		}
		const DxbcInspector inspector;
		auto varianceContainer = inspector.Inspect(ByteView{ withVariance->data(), withVariance->size() });
		if (!varianceContainer) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, varianceContainer.error().Offset });
		}
		const std::size_t exponentInsertion = kExponentInsertionTokenOffset + varianceTokens.size();
		if (!HasInstruction(*varianceContainer, exponentInsertion, kOpcodeDp3, 7)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, exponentInsertion });
		}
		auto corrected = InsertTokens(*varianceContainer, exponentInsertion, exponentTokens);
		if (!corrected) {
			return std::unexpected(corrected.error());
		}

		auto recheck = inspector.Inspect(ByteView{ corrected->data(), corrected->size() });
		if (!recheck) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, recheck.error().Offset });
		}
		const auto newTemps = FindTemps(*recheck);
		if (corrected->size() != original.size() + kInsertedByteCount ||
			recheck->Instructions().size() != container.Instructions().size() + 18 ||
			recheck->ShaderTokenCount() != container.ShaderTokenCount() + kInsertedTokenCount ||
			!newTemps || *newTemps != kOriginalTempCount || !HasSpecularAA(*recheck)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, kCullTokenOffset });
		}

		return DxbcIsotropicSpecularAAPatchResult{
			.Bytes = std::move(*corrected),
			.VarianceInsertionTokenOffset = kCullTokenOffset,
			.VarianceInsertionFileOffset = container.ShaderStreamFileOffset() + kCullTokenOffset * sizeof(std::uint32_t),
			.ExponentOriginalTokenOffset = kExponentInsertionTokenOffset,
			.ExponentOriginalFileOffset = container.ShaderStreamFileOffset() + kExponentInsertionTokenOffset * sizeof(std::uint32_t),
			.InsertedTokenCount = kInsertedTokenCount,
			.OriginalTempCount = *temps,
		};
	}
	catch (...) {
		return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
	}
}
