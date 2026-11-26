#pragma once

#include "Core/ShaderIdentity.h"

#include <span>

namespace NaNBuster
{
	enum class FixId
	{
		Color0LogClamp,
		LambertDecodeGuard,
		AnisotropicSpecularClamp,
		LambertDecodeGuardWithAnisotropicSpecularClamp,
		IsotropicSpecularAA,
	};

	struct FixPolicy
	{
		bool Color0LogClampEnabled = true;
		bool LambertDecodeGuardEnabled = true;
		bool AnisotropicSpecularClampEnabled = true;
		bool IsotropicSpecularAAEnabled = true;

		[[nodiscard]] static constexpr auto AllEnabled() noexcept -> FixPolicy
		{
			return {};
		}

		[[nodiscard]] constexpr auto IsEnabled(FixId fix) const noexcept -> bool
		{
			switch (fix) {
			case FixId::Color0LogClamp:
				return Color0LogClampEnabled;
			case FixId::LambertDecodeGuard:
				return LambertDecodeGuardEnabled;
			case FixId::AnisotropicSpecularClamp:
				return AnisotropicSpecularClampEnabled;
			case FixId::LambertDecodeGuardWithAnisotropicSpecularClamp:
				return LambertDecodeGuardEnabled && AnisotropicSpecularClampEnabled;
			case FixId::IsotropicSpecularAA:
				return IsotropicSpecularAAEnabled;
			}
			return false;
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
