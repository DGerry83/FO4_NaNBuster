#pragma once

#include "Application/ShaderPatchRegistry.h"

#include <filesystem>

namespace NaNBuster
{
	struct IniConfigLoadResult
	{
		FixPolicy Policy = FixPolicy::AllEnabled();
		bool PathError = false;
		bool Color0LogClampInvalid = false;
		bool LambertDecodeGuardInvalid = false;
		bool AnisotropicSpecularClampInvalid = false;
		bool IsotropicSpecularAAInvalid = false;
	};

	class IniConfigLoader final
	{
	public:
		[[nodiscard]] static auto DefaultPath() noexcept -> std::filesystem::path;
		[[nodiscard]] static auto Load(const std::filesystem::path& path) noexcept -> IniConfigLoadResult;
	};
}
