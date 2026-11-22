#pragma once

#include "Core/ShaderIdentity.h"

#include <cstdint>

namespace NaNBuster
{
	// Engine-independent result of one original engine shader-creation call.
	// EngineResult is the native post-device-call result (HRESULT/eax; S_OK is
	// zero); ShaderHandle is the engine wrapper pointer. Application never
	// dereferences ShaderHandle.
	struct EngineCreationOutcome
	{
		std::intptr_t EngineResult;
		void* ShaderHandle;

		auto Succeeded() const noexcept -> bool
		{
			return EngineResult == 0;
		}
	};

	// Locked C11 creation boundary (INTEGRATION_CONTRACT): replay the original
	// engine creation for the selected bytes over an opaque native context that
	// is valid only for the duration of Create. Application exposes Core and
	// standard types only; no ID3D11Device, HRESULT, F4SE, Windows, logging, or
	// engine type crosses this boundary, and creationContext is never
	// dereferenced here.
	class IEngineShaderCreator
	{
	public:
		virtual ~IEngineShaderCreator() = default;

		virtual auto Create(ByteView selectedBytecode, void* creationContext) noexcept
			-> EngineCreationOutcome = 0;
	};
}
