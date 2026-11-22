#pragma once

#include "Application/IEngineShaderCreator.h"

#include <cstdint>
#include <d3d11.h>

namespace NaNBuster
{
	// C10 G5_HOOK_CONTRACT.md §6.3 layout, captured by the hook entry at
	// 0x141d10e82. Private Infrastructure type; valid only for the duration of
	// IEngineShaderCreator::Create. Application never dereferences it; C11/C12
	// use it only to replay the exact original device call. Engine-owned
	// wrapper, device, stream, and pool buffer stay engine-owned in all paths.
	struct OgCreationContext
	{
		ID3D11Device* device;         // from global 0x14609bf88 (re-read by trampoline anyway)
		void* engineWrapper;          // rdi — BSGraphics::PixelShader wrapper
		const void* originalBytecode; // rbx — raw DXBC buffer
		std::uint32_t originalSize;   // [rsp+0x60]
		void* outShaderSlot;          // &wrapper+8 — device-call out-param ([rsp+0x20])
	};

	// Injected original-creation replay. C12 supplies the trampoline-specific
	// callback; it performs exactly one original device call for the selected
	// bytes and returns the post-call HRESULT/eax value. Must not throw.
	using OriginalCreationCallback = std::intptr_t (*)(
		ByteView selectedBytecode, const OgCreationContext& context) noexcept;

	// Original engine creation adapter: stores only the injected callback and
	// has no global state. Each Create forwards the selected view and the exact
	// context to the callback exactly once, then maps the native result and the
	// engine wrapper into EngineCreationOutcome. No validation, retry, byte
	// substitution, allocation, logging, or resource action; views and context
	// are not retained after return.
	class EngineShaderCreator final : public IEngineShaderCreator
	{
	public:
		explicit EngineShaderCreator(OriginalCreationCallback callback) noexcept;

		auto Create(ByteView selectedBytecode, void* creationContext) noexcept
			-> EngineCreationOutcome override;

	private:
		OriginalCreationCallback m_callback;
	};
}
