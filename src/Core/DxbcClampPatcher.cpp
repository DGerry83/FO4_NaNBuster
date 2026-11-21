#include "Core/DxbcClampPatcher.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "Core/DxbcInspector.h"
#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	namespace
	{
		// DXBC container field offsets (bytes); same layout evidence chain as
		// DxbcInspector.cpp (G2_DXBC_EVIDENCE.md section 2).
		constexpr std::size_t kHashOffset = 4;
		constexpr std::size_t kDeclaredTotalOffset = 24;
		constexpr std::size_t kChunkTableOffset = 32;
		constexpr std::size_t kChunkHeaderSize = 8;

		constexpr std::uint32_t kShexFourCC = 0x58454853u; // "SHEX"
		constexpr std::uint32_t kShdrFourCC = 0x52444853u; // "SHDR"

		// D3D10_SB_OPCODE_TYPE values from the local SDK header
		// d3d11TokenizedProgramFormat.hpp: MAX = 52 (line 188), LOG = 47
		// (line 183); anchors re-verified against G2 pins (EXP=25, MUL=56,
		// RET=62) in this session.
		constexpr std::uint32_t kOpcodeMax = 52;
		constexpr std::uint32_t kOpcodeLog = 47;

		constexpr std::uint32_t kInsertedTokenCount = 10;
		constexpr std::uint32_t kInsertedByteCount = kInsertedTokenCount * sizeof(std::uint32_t);
		constexpr std::uint32_t kLogTokenCount = 5;

		// Canonical insertion, max r0.xyzw, v2.xyzw, l(0,0,0,0): exactly ten
		// DWORDs, no extended-opcode or saturate bits. Operand encodings are
		// the original LOG/MUL token shapes (G2_DXBC_EVIDENCE.md section 4).
		constexpr std::uint32_t kInsertedTokens[kInsertedTokenCount] = {
			0x0A000034u, // MAX, declared instruction length 10 DWORDs
			0x001000F2u, 0x00000000u, // dest: TEMP r0, mask x|y|z|w
			0x00101E46u, 0x00000002u, // src: INPUT v2, swizzle xyzw
			0x00004002u,              // imm32 operand, 4 components
			0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
		};

		// Rewritten LOG source: TEMP r0, identity swizzle (matches the EXP
		// source encoding in the original stream). The expected pre-rewrite
		// LOG operand tokens are validated before the rewrite.
		constexpr std::uint32_t kRewrittenLogSource[2] = { 0x00100E46u, 0x00000000u };
		constexpr std::uint32_t kExpectedLogDest[2] = { 0x001000F2u, 0x00000000u };
		constexpr std::uint32_t kExpectedLogSource[2] = { 0x00101E46u, 0x00000002u };

		constexpr std::uint32_t kChannelMaskAll = 0x0F;

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
	}

	auto DxbcClampPatcher::Patch(const DxbcContainerView& container) const
		-> std::expected<DxbcPatchResult, DxbcError>
	try
	{
		const ByteView original = container.OriginalBytes();
		const ByteView stream = container.ShaderStream();
		const std::size_t streamDwords = container.ShaderTokenCount();
		const std::size_t streamFileOffset = container.ShaderStreamFileOffset();

		// Already-clamped detection first (bounded scan over the validated
		// instruction descriptors; every descriptor span is in-stream), so an
		// already transformed container rejects explicitly instead of falling
		// through to the size/hash/region gates.
		for (const auto& instruction : container.Instructions()) {
			if (instruction.Opcode != kOpcodeMax || instruction.TokenLength != kInsertedTokenCount) {
				continue;
			}
			bool matches = true;
			for (std::uint32_t i = 0; i < kInsertedTokenCount; ++i) {
				if (ReadU32(stream.data() + (instruction.TokenOffset + i) * sizeof(std::uint32_t)) !=
					kInsertedTokens[i]) {
					matches = false;
					break;
				}
			}
			if (matches) {
				return std::unexpected(DxbcError{ DxbcErrorCode::AlreadyPatched, instruction.TokenOffset });
			}
		}

		// Exact-original gates: byte size 1612, raw embedded hash at offset 4.
		if (original.size() != static_cast<std::size_t>(OriginalTargetIdentity.BytecodeSize)) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetSizeMismatch, kDeclaredTotalOffset });
		}
		for (std::size_t i = 0; i < OriginalTargetIdentity.EmbeddedHash.size(); ++i) {
			if (original[kHashOffset + i] != OriginalTargetIdentity.EmbeddedHash[i]) {
				return std::unexpected(DxbcError{ DxbcErrorCode::HashMismatch, kHashOffset });
			}
		}

		// Validated target region gate: present, r0 destination, v2 source,
		// full four-channel mask.
		const auto region = container.TargetRegion();
		if (!region.has_value() || region->DestinationRegister != 0u ||
			region->SourceRegister != 2u || region->ChannelMask != kChannelMaskAll) {
			return std::unexpected(DxbcError{
				DxbcErrorCode::TargetRegionMissing,
				region.has_value() ? region->TokenOffset : 0 });
		}

		// The region is anchored on the validated LOG instruction; verify the
		// descriptor and the exact five LOG tokens before rewriting.
		if (region->TokenLength != kLogTokenCount ||
			region->TokenOffset + kLogTokenCount > streamDwords) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, region->TokenOffset });
		}
		bool logDescriptorValid = false;
		for (const auto& instruction : container.Instructions()) {
			if (instruction.TokenOffset == region->TokenOffset) {
				logDescriptorValid =
					instruction.Opcode == kOpcodeLog && instruction.TokenLength == kLogTokenCount;
				break;
			}
		}
		const std::byte* logTokens = stream.data() + region->TokenOffset * sizeof(std::uint32_t);
		if (!logDescriptorValid ||
			ReadU32(logTokens + 1 * sizeof(std::uint32_t)) != kExpectedLogDest[0] ||
			ReadU32(logTokens + 2 * sizeof(std::uint32_t)) != kExpectedLogDest[1] ||
			ReadU32(logTokens + 3 * sizeof(std::uint32_t)) != kExpectedLogSource[0] ||
			ReadU32(logTokens + 4 * sizeof(std::uint32_t)) != kExpectedLogSource[1]) {
			return std::unexpected(DxbcError{ DxbcErrorCode::TargetRegionMissing, region->TokenOffset });
		}

		const DxbcChunkDescriptor* shaderChunk = nullptr;
		for (const auto& chunk : container.Chunks()) {
			if (chunk.FourCC == kShexFourCC || chunk.FourCC == kShdrFourCC) {
				shaderChunk = &chunk;
				break;
			}
		}
		if (shaderChunk == nullptr) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
		}

		// Checked layout arithmetic for the corrected container.
		if (original.size() > std::numeric_limits<std::uint32_t>::max() - kInsertedByteCount) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, kDeclaredTotalOffset });
		}
		const std::size_t newTotal = original.size() + kInsertedByteCount;
		const std::size_t insertionFileOffset =
			streamFileOffset + region->TokenOffset * sizeof(std::uint32_t);

		std::vector<std::byte> corrected;
		corrected.reserve(newTotal);
		corrected.insert(corrected.end(), original.begin(), original.begin() + insertionFileOffset);
		for (std::uint32_t i = 0; i < kInsertedTokenCount; ++i) {
			const std::size_t at = corrected.size();
			corrected.resize(at + sizeof(std::uint32_t));
			WriteU32(corrected.data() + at, kInsertedTokens[i]);
		}
		const std::size_t rewrittenLogAt = corrected.size();
		for (std::uint32_t i = 0; i < kLogTokenCount; ++i) {
			const std::size_t at = corrected.size();
			corrected.resize(at + sizeof(std::uint32_t));
			WriteU32(corrected.data() + at, ReadU32(logTokens + i * sizeof(std::uint32_t)));
		}
		WriteU32(corrected.data() + rewrittenLogAt + 3 * sizeof(std::uint32_t), kRewrittenLogSource[0]);
		WriteU32(corrected.data() + rewrittenLogAt + 4 * sizeof(std::uint32_t), kRewrittenLogSource[1]);
		corrected.insert(corrected.end(),
			original.begin() + insertionFileOffset + kLogTokenCount * sizeof(std::uint32_t),
			original.end());

		// Header and chunk-table fixups: declared total size, shader-chunk
		// content size, shader DWORD length, and chunk-table entries pointing
		// after the shader chunk. Preceding chunks keep their offsets;
		// ordering and payloads are otherwise untouched.
		WriteU32(corrected.data() + kDeclaredTotalOffset, static_cast<std::uint32_t>(newTotal));
		for (std::size_t i = 0; i < container.Chunks().size(); ++i) {
			const DxbcChunkDescriptor& chunk = container.Chunks()[i];
			if (chunk.Offset > shaderChunk->Offset) {
				const std::size_t entry = kChunkTableOffset + i * sizeof(std::uint32_t);
				WriteU32(corrected.data() + entry,
					static_cast<std::uint32_t>(ReadU32(corrected.data() + entry) + kInsertedByteCount));
			}
		}
		WriteU32(corrected.data() + shaderChunk->Offset + 4,
			static_cast<std::uint32_t>(shaderChunk->Size + kInsertedByteCount));
		const std::size_t shaderLengthField = shaderChunk->Offset + kChunkHeaderSize + sizeof(std::uint32_t);
		WriteU32(corrected.data() + shaderLengthField,
			static_cast<std::uint32_t>(streamDwords + 2 + kInsertedTokenCount));

		// Reinspect the owned corrected bytes before returning success.
		const DxbcInspector inspector;
		auto recheck = inspector.Inspect(ByteView{ corrected.data(), corrected.size() });
		if (!recheck.has_value()) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, recheck.error().Offset });
		}
		if (recheck->Instructions().size() != container.Instructions().size() + 1 ||
			recheck->ShaderTokenCount() != streamDwords + kInsertedTokenCount ||
			recheck->TargetRegion().has_value()) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, region->TokenOffset });
		}
		bool insertedVerified = false;
		for (const auto& instruction : recheck->Instructions()) {
			if (instruction.TokenOffset == region->TokenOffset) {
				insertedVerified =
					instruction.Opcode == kOpcodeMax && instruction.TokenLength == kInsertedTokenCount;
				break;
			}
		}
		if (!insertedVerified) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, region->TokenOffset });
		}

		DxbcPatchResult result;
		result.Bytes = std::move(corrected);
		result.InsertionTokenOffset = region->TokenOffset;
		result.InsertionFileOffset = insertionFileOffset;
		result.LogTokenOffset = region->TokenOffset + kInsertedTokenCount;
		result.LogFileOffset = insertionFileOffset + kInsertedByteCount;
		result.InsertedByteCount = kInsertedByteCount;
		return result;
	}
	catch (...) {
		return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, 0 });
	}
}
