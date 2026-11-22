#include "Infrastructure/EmbeddedShaderPayload.h"

#include <array>
#include <cstddef>
#include <cstdint>

// Data-only generated declarations (NaNBuster::Generated::CorrectedShaderDxbc
// and CorrectedShaderDxbcSize); this translation unit supplies the <array>,
// <cstddef>, and <cstdint> declarations the generated include requires.
#include "patched.inc"

namespace NaNBuster
{
	auto EmbeddedShaderPayload::GetCorrectedBytecode() const noexcept -> ByteView
	{
		return std::as_bytes(std::span{ Generated::CorrectedShaderDxbc });
	}

	auto EmbeddedShaderPayload::GetSourceIdentity() const noexcept -> const ShaderIdentity&
	{
		return OriginalTargetIdentity;
	}
}
