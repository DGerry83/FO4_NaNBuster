#pragma once

#include "Application/IShaderPayload.h"

namespace NaNBuster
{
	// Process-lifetime immutable adapter over the C6-generated corrected DXBC
	// (patched.inc). The adapter holds no state: the generated array is an
	// inline constexpr translation-unit value that outlives every adapter
	// instance and every ByteView handed out, so views remain valid after any
	// single adapter is destroyed. Implements the C8 payload contract with
	// engine-agnostic standard C++ only; no F4SE, D3D, filesystem, or logging
	// types. The source identity is exactly OriginalTargetIdentity.
	class EmbeddedShaderPayload final : public IShaderPayload
	{
	public:
		EmbeddedShaderPayload() noexcept = default;

		auto GetCorrectedBytecode() const noexcept -> ByteView override;
		auto GetSourceIdentity() const noexcept -> const ShaderIdentity& override;
	};
}
