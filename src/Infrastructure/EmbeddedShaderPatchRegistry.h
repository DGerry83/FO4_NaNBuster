#pragma once

#include "Application/ShaderPatchRegistry.h"

namespace NaNBuster
{
	class EmbeddedShaderPatchRegistry final : public IShaderPatchRegistry
	{
	public:
		[[nodiscard]] auto Entries() const noexcept -> std::span<const ShaderPatchEntry> override;
	};
}
