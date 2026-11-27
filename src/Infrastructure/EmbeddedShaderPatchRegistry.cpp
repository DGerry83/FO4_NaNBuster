#include "Infrastructure/EmbeddedShaderPatchRegistry.h"

#include "Core/AEShaderVariantIdentities.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "Core/LambertShaderIdentities.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "262c37a4-f4966781-271cf8ba-64c06ec1/anisotropic-clamped.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/combined.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/anisotropic-clamped.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/combined.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "4bfc847c-48cbb854-83d96138-488b9bb1/patched.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/anisotropic-clamped.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/combined.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/anisotropic-clamped.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/combined.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/anisotropic-clamped.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/combined.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/anisotropic-clamped.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/combined.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/anisotropic-clamped.inc"
#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/combined.inc"
#include "AEShaderVariants/262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/anisotropic-clamped.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/combined.inc"
#include "AEShaderVariants/325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "AEShaderVariants/4bfc847c-48cbb854-83d96138-488b9bb1/patched.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/anisotropic-clamped.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/combined.inc"
#include "AEShaderVariants/4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/anisotropic-clamped.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/combined.inc"
#include "AEShaderVariants/600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/anisotropic-clamped.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/combined.inc"
#include "AEShaderVariants/6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/anisotropic-clamped.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/combined.inc"
#include "AEShaderVariants/8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "AEShaderVariants/bcc0320d-dee924ba-4d26b206-84d74f3f/patched.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/anisotropic-clamped.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/combined.inc"
#include "AEShaderVariants/be21279d-733505d8-43f98a86-09502d65/patched.inc"
#include "AEShaderVariants/d75dfae6-af7b9aa5-1a0374d8-3d186619/patched.inc"
#include "AEShaderVariants/def41f6e-9dbcf63b-4c43cf22-4eea0bb4/patched.inc"
#include "AEShaderVariants/e73d9a26-0c9afe98-f628f975-0e5f8aef/patched.inc"
#include "AEShaderVariants/f4b2da50-c279fcd9-bd60ba4f-6a804632/patched.inc"
#include "IsotropicSpecularAA/f4b2da50-c279fcd9-bd60ba4f-6a804632/patched.inc"
#include "bcc0320d-dee924ba-4d26b206-84d74f3f/patched.inc"
#include "be21279d-733505d8-43f98a86-09502d65/anisotropic-clamped.inc"
#include "be21279d-733505d8-43f98a86-09502d65/combined.inc"
#include "be21279d-733505d8-43f98a86-09502d65/patched.inc"
#include "d75dfae6-af7b9aa5-1a0374d8-3d186619/patched.inc"
#include "def41f6e-9dbcf63b-4c43cf22-4eea0bb4/patched.inc"
#include "e73d9a26-0c9afe98-f628f975-0e5f8aef/patched.inc"
#include "patched.inc"

namespace NaNBuster
{
namespace
{
template <std::size_t N> constexpr auto AsByteView(const std::array<std::uint8_t, N> &bytes) -> ByteView
{
	return ByteView{reinterpret_cast<const std::byte *>(bytes.data()), bytes.size()};
}

const std::array kEntries{
	ShaderPatchEntry{FixId::Color0LogClamp, &OriginalTargetIdentity, AsByteView(Generated::CorrectedShaderDxbc)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[1].Identity,
					 AsByteView(Generated::Combined_8904fc0f)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[1].Identity,
					 AsByteView(Generated::Anisotropic_8904fc0f)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[11].Identity,
					 AsByteView(Generated::Combined_be21279d)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[11].Identity,
					 AsByteView(Generated::Anisotropic_be21279d)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[2].Identity,
					 AsByteView(Generated::Combined_4dc83306)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[2].Identity,
					 AsByteView(Generated::Anisotropic_4dc83306)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[3].Identity,
					 AsByteView(Generated::Combined_325742ce)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[3].Identity,
					 AsByteView(Generated::Anisotropic_325742ce)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[5].Identity,
					 AsByteView(Generated::Combined_600e22f5)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[5].Identity,
					 AsByteView(Generated::Anisotropic_600e22f5)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[7].Identity,
					 AsByteView(Generated::Combined_6b7aa1fd)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[7].Identity,
					 AsByteView(Generated::Anisotropic_6b7aa1fd)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &LambertShaderIdentities[9].Identity,
					 AsByteView(Generated::Combined_262c37a4)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &LambertShaderIdentities[9].Identity,
					 AsByteView(Generated::Anisotropic_262c37a4)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[0].Identity,
					 AsByteView(Generated::Lambert_bcc0320d)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[1].Identity,
					 AsByteView(Generated::Lambert_8904fc0f)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[2].Identity,
					 AsByteView(Generated::Lambert_4dc83306)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[3].Identity,
					 AsByteView(Generated::Lambert_325742ce)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[4].Identity,
					 AsByteView(Generated::Lambert_def41f6e)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[5].Identity,
					 AsByteView(Generated::Lambert_600e22f5)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[6].Identity,
					 AsByteView(Generated::Lambert_d75dfae6)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[7].Identity,
					 AsByteView(Generated::Lambert_6b7aa1fd)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[8].Identity,
					 AsByteView(Generated::Lambert_4bfc847c)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[9].Identity,
					 AsByteView(Generated::Lambert_262c37a4)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[10].Identity,
					 AsByteView(Generated::Lambert_e73d9a26)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &LambertShaderIdentities[11].Identity,
					 AsByteView(Generated::Lambert_be21279d)},
	ShaderPatchEntry{FixId::IsotropicSpecularAA, &IsotropicSpecularTargetIdentity,
					 AsByteView(Generated::IsotropicSpecularAA_f4b2da50)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[1].Identity,
					 AsByteView(Generated::AE_Combined_8904fc0f)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[1].Identity,
					 AsByteView(Generated::AE_Anisotropic_8904fc0f)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[11].Identity,
					 AsByteView(Generated::AE_Combined_be21279d)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[11].Identity,
					 AsByteView(Generated::AE_Anisotropic_be21279d)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[2].Identity,
					 AsByteView(Generated::AE_Combined_4dc83306)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[2].Identity,
					 AsByteView(Generated::AE_Anisotropic_4dc83306)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[3].Identity,
					 AsByteView(Generated::AE_Combined_325742ce)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[3].Identity,
					 AsByteView(Generated::AE_Anisotropic_325742ce)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[5].Identity,
					 AsByteView(Generated::AE_Combined_600e22f5)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[5].Identity,
					 AsByteView(Generated::AE_Anisotropic_600e22f5)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[7].Identity,
					 AsByteView(Generated::AE_Combined_6b7aa1fd)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[7].Identity,
					 AsByteView(Generated::AE_Anisotropic_6b7aa1fd)},
	ShaderPatchEntry{FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &AELambertShaderIdentities[9].Identity,
					 AsByteView(Generated::AE_Combined_262c37a4)},
	ShaderPatchEntry{FixId::AnisotropicSpecularClamp, &AELambertShaderIdentities[9].Identity,
					 AsByteView(Generated::AE_Anisotropic_262c37a4)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[0].Identity,
					 AsByteView(Generated::AE_Lambert_bcc0320d)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[1].Identity,
					 AsByteView(Generated::AE_Lambert_8904fc0f)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[2].Identity,
					 AsByteView(Generated::AE_Lambert_4dc83306)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[3].Identity,
					 AsByteView(Generated::AE_Lambert_325742ce)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[4].Identity,
					 AsByteView(Generated::AE_Lambert_def41f6e)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[5].Identity,
					 AsByteView(Generated::AE_Lambert_600e22f5)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[6].Identity,
					 AsByteView(Generated::AE_Lambert_d75dfae6)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[7].Identity,
					 AsByteView(Generated::AE_Lambert_6b7aa1fd)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[8].Identity,
					 AsByteView(Generated::AE_Lambert_4bfc847c)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[9].Identity,
					 AsByteView(Generated::AE_Lambert_262c37a4)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[10].Identity,
					 AsByteView(Generated::AE_Lambert_e73d9a26)},
	ShaderPatchEntry{FixId::LambertDecodeGuard, &AELambertShaderIdentities[11].Identity,
					 AsByteView(Generated::AE_Lambert_be21279d)},
	ShaderPatchEntry{FixId::IsotropicSpecularAA, &AEIsotropicSpecularTargetIdentity,
					 AsByteView(Generated::AE_IsotropicSpecularAA_f4b2da50)},
};
} // namespace

auto EmbeddedShaderPatchRegistry::Entries() const noexcept -> std::span<const ShaderPatchEntry>
{
	return kEntries;
}
} // namespace NaNBuster
