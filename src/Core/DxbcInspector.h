#pragma once

#include <expected>

#include "Core/DxbcContainer.h"
#include "Core/ShaderIdentity.h"

namespace NaNBuster
{
	// Validates a DXBC container and its SHEX/SHDR token stream, producing a
	// non-owning DxbcContainerView or a descriptive DxbcError. Performs no
	// unchecked pointer or offset arithmetic; malformed input always yields an
	// error, never a crash or throw. Standard-library only.
	class DxbcInspector
	{
	public:
		auto Inspect(ByteView bytes) const -> std::expected<DxbcContainerView, DxbcError>;
	};
}
