#pragma once

#include "Core/ShaderIdentity.h"

#include <span>

namespace NaNBuster
{
	enum class FixId
	{
		Color0LogClamp,
		LambertDecodeGuard,
	};

	struct FixPolicy
	{
		bool Color0LogClampEnabled = true;
		bool LambertDecodeGuardEnabled = true;

		[[nodiscard]] static constexpr auto AllEnabled() noexcept -> FixPolicy
		{
			return {};
		}

		[[nodiscard]] constexpr auto IsEnabled(FixId fix) const noexcept -> bool
		{
			return fix == FixId::Color0LogClamp ? Color0LogClampEnabled : LambertDecodeGuardEnabled;
		}
	};

	struct ShaderPatchEntry
	{
		FixId Fix;
		const ShaderIdentity* SourceIdentity;
		ByteView CorrectedBytecode;
	};

	class IShaderPatchRegistry
	{
	public:
		virtual ~IShaderPatchRegistry() = default;

		[[nodiscard]] virtual auto Entries() const noexcept -> std::span<const ShaderPatchEntry> = 0;
	};
}
