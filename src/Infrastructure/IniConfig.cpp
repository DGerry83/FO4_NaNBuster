#include "Infrastructure/IniConfig.h"

#include <array>
#include <cwctype>
#include <string_view>

#include <windows.h>

namespace NaNBuster
{
	namespace
	{
		struct Setting
		{
			bool Present = false;
			bool Valid = false;
			bool Value = true;
		};

		auto Trim(std::wstring_view value) -> std::wstring_view
		{
			while (!value.empty() && std::iswspace(value.front())) {
				value.remove_prefix(1);
			}
			while (!value.empty() && std::iswspace(value.back())) {
				value.remove_suffix(1);
			}
			return value;
		}

		auto EqualsInsensitive(std::wstring_view left, std::wstring_view right) -> bool
		{
			if (left.size() != right.size()) {
				return false;
			}
			for (std::size_t i = 0; i < left.size(); ++i) {
				if (std::towlower(left[i]) != std::towlower(right[i])) {
					return false;
				}
			}
			return true;
		}

		auto ReadSetting(const std::filesystem::path& path, const wchar_t* key) noexcept -> Setting
		{
			constexpr wchar_t missing[] = L"\x1";
			std::array<wchar_t, 64> buffer{};
			const auto count = GetPrivateProfileStringW(
				L"Fixes", key, missing, buffer.data(), static_cast<DWORD>(buffer.size()), path.c_str());
			const std::wstring_view value{ buffer.data(), count };
			if (value == missing) {
				return {};
			}

			const auto trimmed = Trim(value);
			if (EqualsInsensitive(trimmed, L"true") || trimmed == L"1") {
				return { true, true, true };
			}
			if (EqualsInsensitive(trimmed, L"false") || trimmed == L"0") {
				return { true, true, false };
			}
			return { true, false, true };
		}
	}

	auto IniConfigLoader::DefaultPath() noexcept -> std::filesystem::path
	try
	{
		HMODULE module = nullptr;
		if (GetModuleHandleExW(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&IniConfigLoader::DefaultPath), &module) == 0) {
			return {};
		}

		std::array<wchar_t, MAX_PATH> buffer{};
		const auto length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0 || length >= buffer.size()) {
			return {};
		}
		auto path = std::filesystem::path{ std::wstring_view{ buffer.data(), length } };
		path.replace_filename("NaNBuster.ini");
		return path;
	}
	catch (...) {
		return {};
	}

	auto IniConfigLoader::Load(const std::filesystem::path& path) noexcept -> IniConfigLoadResult
	try
	{
		IniConfigLoadResult result;
		if (path.empty()) {
			result.PathError = true;
			return result;
		}

		const auto color = ReadSetting(path, L"Color0LogClamp");
		const auto lambert = ReadSetting(path, L"LambertDecodeGuard");
		const auto anisotropic = ReadSetting(path, L"AnisotropicSpecularClamp");
		const auto isotropic = ReadSetting(path, L"IsotropicSpecularAA");
		result.Policy.Color0LogClampEnabled = !color.Present || !color.Valid || color.Value;
		result.Policy.LambertDecodeGuardEnabled = !lambert.Present || !lambert.Valid || lambert.Value;
		result.Policy.AnisotropicSpecularClampEnabled = !anisotropic.Present || !anisotropic.Valid || anisotropic.Value;
		result.Policy.IsotropicSpecularAAEnabled = !isotropic.Present || !isotropic.Valid || isotropic.Value;
		result.Color0LogClampInvalid = color.Present && !color.Valid;
		result.LambertDecodeGuardInvalid = lambert.Present && !lambert.Valid;
		result.AnisotropicSpecularClampInvalid = anisotropic.Present && !anisotropic.Valid;
		result.IsotropicSpecularAAInvalid = isotropic.Present && !isotropic.Valid;
		return result;
	}
	catch (...) {
		IniConfigLoadResult result;
		result.PathError = true;
		return result;
	}
}
