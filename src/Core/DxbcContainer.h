#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	// Token offsets and counts are measured in DWORD (4-byte) units, indexed
	// from the start of the SHEX/SHDR token stream (the first token after the
	// shader version and length DWORDs). Container offsets are byte offsets
	// from the start of the DXBC container.
	enum class DxbcErrorCode
	{
		BadMagic,
		HeaderTruncated,
		DeclaredSizeMismatch,
		ChunkCountOverflow,
		ChunkTableOutOfRange,
		ChunkHeaderOutOfRange,
		ChunkOutOfRange,
		ChunksOverlap,
		MissingShaderChunk,
		DuplicateShaderChunk,
		ShaderLengthMismatch,
		UnsupportedCustomData,
		ZeroInstructionLength,
		InstructionOutOfRange,
		InternalError,
		// C4 clamp-patcher rejections (DxbcClampPatcher). Offsets: container
		// byte offset for TargetSizeMismatch (24) and HashMismatch (4),
		// token index for TargetRegionMissing/AlreadyPatched.
		TargetSizeMismatch,
		HashMismatch,
		TargetRegionMissing,
		AlreadyPatched,
	};

	// Error code plus the byte offset (container errors) or DWORD token index
	// (token-stream errors, see DxbcErrorCode docs) that identifies the
	// rejected boundary.
	struct DxbcError
	{
		DxbcErrorCode Code;
		std::size_t Offset;
	};

	// One validated DXBC chunk. Offset is the byte offset of the chunk header
	// (its FourCC); Size is the chunk content size in bytes, excluding the
	// 8-byte FourCC+Size chunk header.
	struct DxbcChunkDescriptor
	{
		std::uint32_t FourCC;
		std::size_t Offset;
		std::size_t Size;
	};

	// One validated token-stream instruction. TokenOffset is the DWORD index
	// of the instruction token within the shader token stream; TokenLength is
	// the declared instruction length in DWORDs including the opcode token(s).
	struct DxbcInstructionDescriptor
	{
		std::size_t TokenOffset;
		std::size_t TokenLength;
		std::uint32_t Opcode;
	};

	// The approved vulnerable sequence anchor, exposed for C4. Per chunk D3
	// ruling: TokenOffset is the LOG instruction start and TokenLength is the
	// LOG instruction length only; the inspector validates that the exact
	// mul/exp pair follows before exposing this region. All values are token
	// indices/counts; use DxbcContainerView::ShaderStreamFileOffset() to map
	// TokenOffset to an absolute container byte offset.
	struct DxbcInstructionRegion
	{
		std::size_t TokenOffset;
		std::size_t TokenLength;
		std::uint32_t DestinationRegister;
		std::uint32_t SourceRegister;
		std::uint8_t ChannelMask;
	};

	// Validated, non-owning view over a DXBC container. Owns the chunk and
	// instruction descriptor vectors; the original bytes, chunk payloads, and
	// token stream are views into caller-owned storage that must outlive this
	// object.
	class DxbcContainerView
	{
	public:
		ByteView OriginalBytes() const noexcept { return m_original; }

		std::span<const DxbcChunkDescriptor> Chunks() const noexcept { return m_chunks; }

		std::span<const DxbcInstructionDescriptor> Instructions() const noexcept
		{
			return m_instructions;
		}

		// Non-owning view over the shader chunk token stream (DWORD aligned).
		ByteView ShaderStream() const noexcept { return m_shaderStream; }

		// Number of DWORD tokens in ShaderStream().
		std::size_t ShaderTokenCount() const noexcept { return m_shaderStream.size() / sizeof(std::uint32_t); }

		// Absolute byte offset of the first shader token within the container.
		std::size_t ShaderStreamFileOffset() const noexcept;

		// Content view of chunk i (excluding its 8-byte chunk header).
		ByteView ChunkPayload(std::size_t index) const noexcept;

		// The approved vulnerable log/mul/exp region, if this container is the
		// approved target sequence.
		std::optional<DxbcInstructionRegion> TargetRegion() const noexcept { return m_region; }

	private:
		friend class DxbcInspector;

		ByteView m_original;
		ByteView m_shaderStream;
		std::vector<DxbcChunkDescriptor> m_chunks;
		std::vector<DxbcInstructionDescriptor> m_instructions;
		std::optional<DxbcInstructionRegion> m_region;
	};
}
