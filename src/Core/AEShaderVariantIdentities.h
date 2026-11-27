#pragma once

#include "Core/LambertShaderIdentities.h"

#include <cstdint>
#include <string_view>

namespace NaNBuster
{
namespace AEShaderIdentityDetail
{
consteval auto MakeIdentity(std::string_view hash, std::uint32_t byteSize) -> ShaderIdentity
{
	return {
		.ShaderUid = hash,
		.BytecodeSize = byteSize,
		.EmbeddedHash = LambertIdentityDetail::EmbeddedHashFromText(hash.substr(0, 35)),
		.GfxBoosterHash = 0,
		.AssemblyHash = 0,
	};
}

consteval auto MakeLambertIdentity(std::string_view hash, std::uint32_t byteSize, LambertShaderStage stage,
								   std::string_view referenceStem) -> LambertShaderIdentity
{
	return {
		.Identity = MakeIdentity(hash, byteSize),
		.Stage = stage,
		.ReferenceStem = referenceStem,
	};
}
} // namespace AEShaderIdentityDetail

inline constexpr std::array AELambertShaderIdentities{
	AEShaderIdentityDetail::MakeLambertIdentity("42c01806-2fff2213-454d5b47-0febbec5-10000004", 4052,
												LambertShaderStage::Pixel, "bcc0320d-dee924ba-4d26b206-84d74f3f"),
	AEShaderIdentityDetail::MakeLambertIdentity("d259278e-5d6465c2-9597f7a5-9167a174-10000204", 5624,
												LambertShaderStage::Pixel, "8904fc0f-cb497158-117bc46d-4e1cf466"),
	AEShaderIdentityDetail::MakeLambertIdentity("62305116-5f1dbb95-5bf460a1-66fc3e36-00000600", 5916,
												LambertShaderStage::Pixel, "4dc83306-024b9074-9c24d67a-fcdee952"),
	AEShaderIdentityDetail::MakeLambertIdentity("a142467d-0ccdc0c4-42599f57-524ba8ef-00008204", 5360,
												LambertShaderStage::Pixel, "325742ce-45bf17d0-f4879324-3637b6aa"),
	AEShaderIdentityDetail::MakeLambertIdentity("8fae50bb-c275fe55-a0b651bd-6b0232cc-00020000", 2500,
												LambertShaderStage::Pixel, "def41f6e-9dbcf63b-4c43cf22-4eea0bb4"),
	AEShaderIdentityDetail::MakeLambertIdentity("fb40c999-0825a3b1-0d1db084-782331d8-00080208", 6868,
												LambertShaderStage::Pixel, "600e22f5-e1a79678-b183a8f0-99de971d"),
	AEShaderIdentityDetail::MakeLambertIdentity("9a38981c-14b2c2d0-83512c06-872c2a92-0c400282", 2240,
												LambertShaderStage::Pixel, "d75dfae6-af7b9aa5-1a0374d8-3d186619"),
	AEShaderIdentityDetail::MakeLambertIdentity("34500494-908064b9-67601b25-87421985-09200202", 25560,
												LambertShaderStage::Pixel, "6b7aa1fd-0175af05-1bf184b7-22dd8846"),
	AEShaderIdentityDetail::MakeLambertIdentity("c249f76f-01471f14-96e7c782-0ab3dea2-00000a20", 3048,
												LambertShaderStage::Pixel, "4bfc847c-48cbb854-83d96138-488b9bb1"),
	AEShaderIdentityDetail::MakeLambertIdentity("190e840a-34210f13-e95f808f-6f3f155a-09220202", 26292,
												LambertShaderStage::Pixel, "262c37a4-f4966781-271cf8ba-64c06ec1"),
	AEShaderIdentityDetail::MakeLambertIdentity("471e194d-c15a3b7c-a377e02c-723a6df5-00000000", 3892,
												LambertShaderStage::Compute, "e73d9a26-0c9afe98-f628f975-0e5f8aef"),
	AEShaderIdentityDetail::MakeLambertIdentity("2184f4ef-f68e8e50-a5e4109f-7c322184-00000001", 6444,
												LambertShaderStage::Compute, "be21279d-733505d8-43f98a86-09502d65"),
};

inline constexpr ShaderIdentity AEIsotropicSpecularTargetIdentity =
	AEShaderIdentityDetail::MakeIdentity("4f02f995-8e33cdb3-ac24ad49-b3a5c778-00080220", 6516);
inline constexpr std::string_view AEIsotropicSpecularReferenceStem = "f4b2da50-c279fcd9-bd60ba4f-6a804632";
} // namespace NaNBuster
