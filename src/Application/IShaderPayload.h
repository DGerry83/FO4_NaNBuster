#pragma once

#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	// Read-only access to the corrected target shader bytecode and its source
	// identity. Implementations own the corrected storage, which must outlive
	// every ByteView handed out; no ownership transfer occurs. Engine-agnostic:
	// no F4SE, D3D, filesystem, or logging types. Getters never throw or
	// allocate; the source identity is the immutable OriginalTargetIdentity.
	class IShaderPayload
	{
	public:
		virtual ~IShaderPayload() = default;

		virtual auto GetCorrectedBytecode() const noexcept -> ByteView = 0;
		virtual auto GetSourceIdentity() const noexcept -> const ShaderIdentity& = 0;
	};
}
