#include "Core/DxbcInspector.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace NaNBuster
{
	namespace
	{
		// DXBC container layout, verified against the original target and the
		// local Windows SDK d3d11TokenizedProgramFormat.hpp evidence
		// (build/shaders/PS49B6A21CI0O1/token-decode.txt).
		constexpr std::size_t kContainerHeaderSize = 32; // magic4 + hash16 + version4 + total4 + count4
		constexpr std::size_t kChunkHeaderSize = 8;      // FourCC4 + content-size4
		constexpr std::size_t kShaderFixedDwords = 2;    // version DWORD + length DWORD

		constexpr std::uint32_t kShexFourCC = 0x58454853u; // "SHEX"
		constexpr std::uint32_t kShdrFourCC = 0x52444853u; // "SHDR"

		// D3D10_SB_TOKENIZED_INSTRUCTION layout (header lines 418-433):
		// opcode bits [10:00], length bits [30:24] in DWORDs including the
		// opcode token(s), range 1-127.
		constexpr std::uint32_t kOpcodeMask = 0x000007FFu;
		constexpr std::uint32_t kLengthMask = 0x7F000000u;
		constexpr std::uint32_t kLengthShift = 24;

		constexpr std::uint32_t kOpcodeCustomData = 53; // D3D10_SB_OPCODE_CUSTOMDATA
		constexpr std::uint32_t kOpcodeLog = 47;        // D3D10_SB_OPCODE_LOG
		constexpr std::uint32_t kOpcodeMul = 56;        // D3D10_SB_OPCODE_MUL
		constexpr std::uint32_t kOpcodeExp = 25;        // D3D10_SB_OPCODE_EXP

		// D3D10_SB_OPERAND_TOKEN0 layout (header lines 692-1016).
		constexpr std::uint32_t kOperandTypeMask = 0x000FF000u;
		constexpr std::uint32_t kOperandTypeShift = 12;
		constexpr std::uint32_t kOperandIndexDimMask = 0x00300000u;
		constexpr std::uint32_t kOperandIndexDimShift = 20;
		constexpr std::uint32_t kOperandExtendedMask = 0x80000000u;

		constexpr std::uint32_t kOperandTypeTemp = 0;
		constexpr std::uint32_t kOperandTypeInput = 1;
		constexpr std::uint32_t kOperandTypeImmediate32 = 4;

		constexpr std::uint8_t kChannelMaskAll = 0x0F; // x|y|z|w
		constexpr std::uint32_t kGammaImmediate = 0x400CCCCDu; // 2.2f bits

		auto ReadU32(const std::byte* p) noexcept -> std::uint32_t
		{
			return static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[0])) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[1])) << 8) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[2])) << 16) |
				(static_cast<std::uint32_t>(std::to_integer<std::uint32_t>(p[3])) << 24);
		}

		struct OperandInfo
		{
			std::uint32_t Type = 0;
			std::uint32_t NumComponents = 0;
			std::uint32_t SelectionMode = 0;
			std::uint32_t Mask = 0;
			bool Extended = false;
			bool HasImmediate32Index = false;
			std::uint32_t Index0 = 0;
			std::uint32_t Immediate[4] = {};
			std::uint32_t ImmediateCount = 0;
		};

		// Byte-wise DWORD reader over the token stream. Chunk offsets come from
		// untrusted container data, so the stream is not guaranteed to be
		// 4-byte aligned; no reinterpret_cast is ever performed.
		class TokenReader
		{
		public:
			TokenReader(const std::byte* base, std::size_t count) noexcept
				: m_base(base), m_count(count)
			{
			}

			auto Count() const noexcept -> std::size_t { return m_count; }
			auto Get(std::size_t index) const noexcept -> std::uint32_t
			{
				return ReadU32(m_base + index * sizeof(std::uint32_t));
			}

		private:
			const std::byte* m_base;
			std::size_t m_count;
		};

		// Bounded operand parse within [begin, end) token indices. Returns
		// std::nullopt on any out-of-range or unsupported encoding; never
		// reads outside the instruction's declared token span.
		auto ParseOperand(const TokenReader& tokens, std::size_t begin, std::size_t end, std::size_t& pos)
			-> std::optional<OperandInfo>
		{
			if (pos >= end) {
				return std::nullopt;
			}
			const std::uint32_t t0 = tokens.Get(pos);
			++pos;

			OperandInfo info;
			info.Type = (t0 & kOperandTypeMask) >> kOperandTypeShift;
			info.NumComponents = t0 & 0x3u;
			info.SelectionMode = (t0 & 0xCu) >> 2;
			info.Mask = (t0 & 0xFF0u) >> 4;
			const std::uint32_t indexDim = (t0 & kOperandIndexDimMask) >> kOperandIndexDimShift;

			for (std::uint32_t dim = 0; dim < indexDim; ++dim) {
				const std::uint32_t rep = (t0 >> (22 + 3 * dim)) & 3u;
				switch (rep) {
				case 0: // IMMEDIATE32 index: 1 DWORD
					if (pos >= end) {
						return std::nullopt;
					}
					if (dim == 0) {
						info.HasImmediate32Index = true;
						info.Index0 = tokens.Get(pos);
					}
					++pos;
					break;
				case 1: // IMMEDIATE64 index: 2 DWORDs
					if (end - pos < 2) {
						return std::nullopt;
					}
					pos += 2;
					break;
				case 2: // RELATIVE index: nested operand
					if (!ParseOperand(tokens, begin, end, pos)) {
						return std::nullopt;
					}
					break;
				case 3: // IMMEDIATE32 + RELATIVE
					if (pos >= end) {
						return std::nullopt;
					}
					++pos;
					if (!ParseOperand(tokens, begin, end, pos)) {
						return std::nullopt;
					}
					break;
				default: // 4: IMMEDIATE64 + RELATIVE
					if (end - pos < 2) {
						return std::nullopt;
					}
					pos += 2;
					if (!ParseOperand(tokens, begin, end, pos)) {
						return std::nullopt;
					}
					break;
				}
			}

			if (info.Type == kOperandTypeImmediate32) {
				const std::uint32_t count = info.NumComponents == 1 ? 1u : (info.NumComponents == 2 ? 4u : 0u);
				if (end - pos < count) {
					return std::nullopt;
				}
				for (std::uint32_t i = 0; i < count; ++i) {
					info.Immediate[i] = tokens.Get(pos + i);
				}
				info.ImmediateCount = count;
				pos += count;
			} else if (info.Type == 5) { // IMMEDIATE64
				const std::uint32_t count = info.NumComponents == 1 ? 2u : (info.NumComponents == 2 ? 8u : 0u);
				if (end - pos < count) {
					return std::nullopt;
				}
				pos += count;
			}

			if (t0 & kOperandExtendedMask) {
				info.Extended = true;
				do {
					if (pos >= end) {
						return std::nullopt;
					}
				} while (tokens.Get(pos++) & kOperandExtendedMask);
			}

			return info;
		}

		auto IsFullMaskRegister(const OperandInfo& operand, std::uint32_t type, std::uint32_t index) noexcept -> bool
		{
			// Full four-channel operands appear either as mask mode with mask
			// x|y|z|w (0xF) or as swizzle mode with the identity xyzw swizzle
			// (x|y<<2|z<<4|w<<6 = 0xE4); the original target uses both shapes.
			const bool fullChannel = operand.NumComponents == 2 &&
				((operand.SelectionMode == 0 && operand.Mask == 0xFu) ||
					(operand.SelectionMode == 1 && operand.Mask == 0xE4u));
			return !operand.Extended && operand.Type == type && fullChannel &&
				operand.HasImmediate32Index && operand.Index0 == index && operand.ImmediateCount == 0;
		}

		// Validates the exact approved vulnerable sequence at instruction
		// indices log/mul/exp: log rN.xyzw, vM.xyzw -> mul rN.xyzw, rN.xyzw,
		// l(2.2,2.2,2.2,2.2) -> exp rN.xyzw, rN.xyzw. Returns the region with
		// TokenOffset/TokenLength anchored on the LOG instruction only.
		auto MatchTargetRegion(
			const TokenReader& tokens,
			const std::vector<DxbcInstructionDescriptor>& instructions,
			std::size_t logIndex) noexcept -> std::optional<DxbcInstructionRegion>
		{
			const DxbcInstructionDescriptor& log = instructions[logIndex];
			const DxbcInstructionDescriptor& mul = instructions[logIndex + 1];
			const DxbcInstructionDescriptor& exp = instructions[logIndex + 2];

			std::size_t pos = log.TokenOffset + 1;
			const std::size_t logEnd = log.TokenOffset + log.TokenLength;
			const auto dest = ParseOperand(tokens, log.TokenOffset, logEnd, pos);
			const auto src = ParseOperand(tokens, log.TokenOffset, logEnd, pos);
			if (!dest || !src || pos != logEnd ||
				!IsFullMaskRegister(*dest, kOperandTypeTemp, 0) ||
				!IsFullMaskRegister(*src, kOperandTypeInput, 2)) {
				return std::nullopt;
			}

			pos = mul.TokenOffset + 1;
			const std::size_t mulEnd = mul.TokenOffset + mul.TokenLength;
			const auto mulDest = ParseOperand(tokens, mul.TokenOffset, mulEnd, pos);
			const auto mulSrc = ParseOperand(tokens, mul.TokenOffset, mulEnd, pos);
			const auto mulImm = ParseOperand(tokens, mul.TokenOffset, mulEnd, pos);
			if (!mulDest || !mulSrc || !mulImm || pos != mulEnd ||
				!IsFullMaskRegister(*mulDest, kOperandTypeTemp, 0) ||
				!IsFullMaskRegister(*mulSrc, kOperandTypeTemp, 0) ||
				mulImm->Extended || mulImm->Type != kOperandTypeImmediate32 ||
				mulImm->NumComponents != 2 || mulImm->ImmediateCount != 4 ||
				mulImm->HasImmediate32Index) {
				return std::nullopt;
			}
			for (std::uint32_t i = 0; i < 4; ++i) {
				if (mulImm->Immediate[i] != kGammaImmediate) {
					return std::nullopt;
				}
			}

			pos = exp.TokenOffset + 1;
			const std::size_t expEnd = exp.TokenOffset + exp.TokenLength;
			const auto expDest = ParseOperand(tokens, exp.TokenOffset, expEnd, pos);
			const auto expSrc = ParseOperand(tokens, exp.TokenOffset, expEnd, pos);
			if (!expDest || !expSrc || pos != expEnd ||
				!IsFullMaskRegister(*expDest, kOperandTypeTemp, 0) ||
				!IsFullMaskRegister(*expSrc, kOperandTypeTemp, 0)) {
				return std::nullopt;
			}

			DxbcInstructionRegion region;
			region.TokenOffset = log.TokenOffset;
			region.TokenLength = log.TokenLength;
			region.DestinationRegister = 0;
			region.SourceRegister = 2;
			region.ChannelMask = kChannelMaskAll;
			return region;
		}
	}

	auto DxbcInspector::Inspect(ByteView bytes) const -> std::expected<DxbcContainerView, DxbcError>
	try
	{
		if (bytes.size() < 4) {
			return std::unexpected(DxbcError{ DxbcErrorCode::HeaderTruncated, 0 });
		}
		if (bytes[0] != std::byte{ 'D' } || bytes[1] != std::byte{ 'X' } ||
			bytes[2] != std::byte{ 'B' } || bytes[3] != std::byte{ 'C' }) {
			return std::unexpected(DxbcError{ DxbcErrorCode::BadMagic, 0 });
		}
		if (bytes.size() < kContainerHeaderSize) {
			return std::unexpected(DxbcError{ DxbcErrorCode::HeaderTruncated, bytes.size() });
		}

		const std::uint32_t declaredTotal = ReadU32(bytes.data() + 24);
		if (declaredTotal != bytes.size()) {
			return std::unexpected(DxbcError{ DxbcErrorCode::DeclaredSizeMismatch, 24 });
		}

		const std::uint32_t chunkCount = ReadU32(bytes.data() + 28);
		if (chunkCount > (static_cast<std::size_t>(-1) - kContainerHeaderSize) / sizeof(std::uint32_t)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::ChunkCountOverflow, 28 });
		}
		const std::size_t tableSize = static_cast<std::size_t>(chunkCount) * sizeof(std::uint32_t);
		if (tableSize > bytes.size() - kContainerHeaderSize) {
			return std::unexpected(DxbcError{ DxbcErrorCode::ChunkTableOutOfRange, kContainerHeaderSize });
		}
		const std::size_t tableEnd = kContainerHeaderSize + tableSize;

		DxbcContainerView view;
		view.m_original = bytes;
		view.m_chunks.reserve(chunkCount);
		for (std::uint32_t i = 0; i < chunkCount; ++i) {
			const std::size_t entryOffset = kContainerHeaderSize + static_cast<std::size_t>(i) * sizeof(std::uint32_t);
			const std::size_t chunkOffset = ReadU32(bytes.data() + entryOffset);
			if (chunkOffset < tableEnd) {
				return std::unexpected(DxbcError{ DxbcErrorCode::ChunkHeaderOutOfRange, chunkOffset });
			}
			if (chunkOffset > bytes.size() || bytes.size() - chunkOffset < kChunkHeaderSize) {
				return std::unexpected(DxbcError{ DxbcErrorCode::ChunkHeaderOutOfRange, chunkOffset });
			}
			const std::uint32_t fourCC = ReadU32(bytes.data() + chunkOffset);
			const std::uint32_t contentSize = ReadU32(bytes.data() + chunkOffset + 4);
			if (contentSize > bytes.size() - chunkOffset - kChunkHeaderSize) {
				return std::unexpected(DxbcError{ DxbcErrorCode::ChunkOutOfRange, chunkOffset });
			}
			view.m_chunks.push_back(DxbcChunkDescriptor{ fourCC, chunkOffset, contentSize });
		}

		// Overlap validation on a sorted copy; Chunks() keeps original order.
		{
			std::vector<std::pair<std::size_t, std::size_t>> spans;
			spans.reserve(view.m_chunks.size());
			for (const auto& chunk : view.m_chunks) {
				spans.emplace_back(chunk.Offset, chunk.Offset + kChunkHeaderSize + chunk.Size);
			}
			std::sort(spans.begin(), spans.end());
			for (std::size_t i = 1; i < spans.size(); ++i) {
				if (spans[i].first < spans[i - 1].second) {
					return std::unexpected(DxbcError{ DxbcErrorCode::ChunksOverlap, spans[i].first });
				}
			}
		}

		// Unique shader chunk (SHEX or SHDR).
		const DxbcChunkDescriptor* shaderChunk = nullptr;
		for (const auto& chunk : view.m_chunks) {
			if (chunk.FourCC == kShexFourCC || chunk.FourCC == kShdrFourCC) {
				if (shaderChunk != nullptr) {
					return std::unexpected(DxbcError{ DxbcErrorCode::DuplicateShaderChunk, chunk.Offset });
				}
				shaderChunk = &chunk;
			}
		}
		if (shaderChunk == nullptr) {
			return std::unexpected(DxbcError{ DxbcErrorCode::MissingShaderChunk, 0 });
		}

		const std::size_t contentOffset = shaderChunk->Offset + kChunkHeaderSize;
		const std::size_t contentSize = shaderChunk->Size;
		if (contentSize < kShaderFixedDwords * sizeof(std::uint32_t) ||
			contentSize % sizeof(std::uint32_t) != 0) {
			return std::unexpected(DxbcError{ DxbcErrorCode::ShaderLengthMismatch, contentOffset });
		}
		const std::uint32_t shaderLengthDwords = ReadU32(bytes.data() + contentOffset + 4);
		if (shaderLengthDwords < kShaderFixedDwords ||
			static_cast<std::size_t>(shaderLengthDwords) * sizeof(std::uint32_t) != contentSize) {
			return std::unexpected(DxbcError{ DxbcErrorCode::ShaderLengthMismatch, contentOffset + 4 });
		}
		const std::size_t streamOffset = contentOffset + kShaderFixedDwords * sizeof(std::uint32_t);
		const std::size_t streamDwords = shaderLengthDwords - kShaderFixedDwords;

		// Instruction-boundary walk over the byte-wise token reader.
		const TokenReader tokens(bytes.data() + streamOffset, streamDwords);
		std::size_t pos = 0;
		while (pos < streamDwords) {
			const std::uint32_t token = tokens.Get(pos);
			const std::uint32_t opcode = token & kOpcodeMask;
			const std::uint32_t length = (token & kLengthMask) >> kLengthShift;
			if (opcode == kOpcodeCustomData) {
				if (pos + 1 >= streamDwords) {
					return std::unexpected(DxbcError{ DxbcErrorCode::InstructionOutOfRange, pos });
				}
				const std::uint32_t blockLength = tokens.Get(pos + 1);
				if (blockLength < 2) {
					return std::unexpected(DxbcError{ DxbcErrorCode::UnsupportedCustomData, pos });
				}
				if (blockLength > streamDwords - pos) {
					return std::unexpected(DxbcError{ DxbcErrorCode::InstructionOutOfRange, pos });
				}
				pos += blockLength;
				continue;
			}
			if (length == 0) {
				return std::unexpected(DxbcError{ DxbcErrorCode::ZeroInstructionLength, pos });
			}
			if (length > streamDwords - pos) {
				return std::unexpected(DxbcError{ DxbcErrorCode::InstructionOutOfRange, pos });
			}
			view.m_instructions.push_back(DxbcInstructionDescriptor{ pos, length, opcode });
			pos += length;
		}

		view.m_shaderStream = bytes.subspan(streamOffset, streamDwords * sizeof(std::uint32_t));

		// Approved vulnerable region: strict operand-level sequence match.
		if (view.m_instructions.size() >= 3) {
			for (std::size_t i = 0; i + 2 < view.m_instructions.size(); ++i) {
				if (view.m_instructions[i].Opcode != kOpcodeLog ||
					view.m_instructions[i + 1].Opcode != kOpcodeMul ||
					view.m_instructions[i + 2].Opcode != kOpcodeExp) {
					continue;
				}
				auto region = MatchTargetRegion(tokens, view.m_instructions, i);
				if (region) {
					view.m_region = *region;
					break;
				}
			}
		}

		return view;
	}
	catch (...) {
		return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
	}

	auto DxbcContainerView::ShaderStreamFileOffset() const noexcept -> std::size_t
	{
		if (m_original.empty() || m_shaderStream.empty()) {
			return 0;
		}
		return static_cast<std::size_t>(m_shaderStream.data() - m_original.data());
	}

	auto DxbcContainerView::ChunkPayload(std::size_t index) const noexcept -> ByteView
	{
		if (index >= m_chunks.size()) {
			return {};
		}
		const DxbcChunkDescriptor& chunk = m_chunks[index];
		return m_original.subspan(chunk.Offset + 8, chunk.Size);
	}
}
