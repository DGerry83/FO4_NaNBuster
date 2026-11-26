#pragma once

#include "Core/ShaderIdentity.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace NaNBuster
{
	enum class LambertShaderStage
	{
		Pixel,
		Compute,
	};

	struct LambertShaderIdentity
	{
		ShaderIdentity Identity;
		LambertShaderStage Stage;
		std::string_view ReferenceStem;
	};

	namespace LambertIdentityDetail
	{
		consteval auto HexDigit(char value) -> std::uint8_t
		{
			if (value >= '0' && value <= '9') {
				return static_cast<std::uint8_t>(value - '0');
			}
			if (value >= 'a' && value <= 'f') {
				return static_cast<std::uint8_t>(value - 'a' + 10);
			}
			if (value >= 'A' && value <= 'F') {
				return static_cast<std::uint8_t>(value - 'A' + 10);
			}
			return 0xFF;
		}

		consteval auto EmbeddedHashFromText(std::string_view text) -> std::array<std::byte, 16>
		{
			if (text.size() != 35 || text[8] != '-' || text[17] != '-' || text[26] != '-') {
				return {};
			}

			constexpr std::array<std::size_t, 4> groupOffsets{ 0, 9, 18, 27 };
			std::array<std::byte, 16> result{};
			for (std::size_t group = 0; group < 4; ++group) {
				for (std::size_t byte = 0; byte < 4; ++byte) {
					const std::size_t displayByte = 3 - byte;
					const std::size_t offset = groupOffsets[group] + displayByte * 2;
					const auto high = HexDigit(text[offset]);
					const auto low = HexDigit(text[offset + 1]);
					if (high > 0x0F || low > 0x0F) {
						return {};
					}
					result[group * 4 + byte] = static_cast<std::byte>((high << 4) | low);
				}
			}
			return result;
		}

		consteval auto MakeIdentity(
			std::string_view hash,
			std::uint32_t byteSize,
			LambertShaderStage stage) -> LambertShaderIdentity
		{
			return {
				.Identity = {
					.ShaderUid = hash,
					.BytecodeSize = byteSize,
					.EmbeddedHash = EmbeddedHashFromText(hash),
					.GfxBoosterHash = 0,
					.AssemblyHash = 0,
				},
				.Stage = stage,
				.ReferenceStem = hash,
			};
		}
	}

	inline constexpr std::array LambertShaderIdentities{
		LambertIdentityDetail::MakeIdentity("bcc0320d-dee924ba-4d26b206-84d74f3f", 3964, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("8904fc0f-cb497158-117bc46d-4e1cf466", 5636, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("4dc83306-024b9074-9c24d67a-fcdee952", 5928, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("325742ce-45bf17d0-f4879324-3637b6aa", 6380, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("def41f6e-9dbcf63b-4c43cf22-4eea0bb4", 2512, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("600e22f5-e1a79678-b183a8f0-99de971d", 6880, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("d75dfae6-af7b9aa5-1a0374d8-3d186619", 2252, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("6b7aa1fd-0175af05-1bf184b7-22dd8846", 25572, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("4bfc847c-48cbb854-83d96138-488b9bb1", 2972, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("262c37a4-f4966781-271cf8ba-64c06ec1", 26304, LambertShaderStage::Pixel),
		LambertIdentityDetail::MakeIdentity("e73d9a26-0c9afe98-f628f975-0e5f8aef", 3892, LambertShaderStage::Compute),
		LambertIdentityDetail::MakeIdentity("be21279d-733505d8-43f98a86-09502d65", 6588, LambertShaderStage::Compute),
	};
}
