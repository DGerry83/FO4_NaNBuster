#include "Infrastructure/EmbeddedShaderPatchRegistry.h"

#include "Core/LambertShaderIdentities.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "patched.inc"
#include "bcc0320d-dee924ba-4d26b206-84d74f3f/patched.inc"
#include "8904fc0f-cb497158-117bc46d-4e1cf466/patched.inc"
#include "4dc83306-024b9074-9c24d67a-fcdee952/patched.inc"
#include "325742ce-45bf17d0-f4879324-3637b6aa/patched.inc"
#include "def41f6e-9dbcf63b-4c43cf22-4eea0bb4/patched.inc"
#include "600e22f5-e1a79678-b183a8f0-99de971d/patched.inc"
#include "d75dfae6-af7b9aa5-1a0374d8-3d186619/patched.inc"
#include "6b7aa1fd-0175af05-1bf184b7-22dd8846/patched.inc"
#include "4bfc847c-48cbb854-83d96138-488b9bb1/patched.inc"
#include "262c37a4-f4966781-271cf8ba-64c06ec1/patched.inc"
#include "e73d9a26-0c9afe98-f628f975-0e5f8aef/patched.inc"

namespace NaNBuster
{
	namespace
	{
		template <std::size_t N>
		constexpr auto AsByteView(const std::array<std::uint8_t, N>& bytes) -> ByteView
		{
			return ByteView{ reinterpret_cast<const std::byte*>(bytes.data()), bytes.size() };
		}

		const std::array kEntries{
			ShaderPatchEntry{ FixId::Color0LogClamp, &OriginalTargetIdentity, AsByteView(Generated::CorrectedShaderDxbc) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[0].Identity, AsByteView(Generated::Lambert_bcc0320d) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[1].Identity, AsByteView(Generated::Lambert_8904fc0f) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[2].Identity, AsByteView(Generated::Lambert_4dc83306) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[3].Identity, AsByteView(Generated::Lambert_325742ce) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[4].Identity, AsByteView(Generated::Lambert_def41f6e) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[5].Identity, AsByteView(Generated::Lambert_600e22f5) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[6].Identity, AsByteView(Generated::Lambert_d75dfae6) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[7].Identity, AsByteView(Generated::Lambert_6b7aa1fd) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[8].Identity, AsByteView(Generated::Lambert_4bfc847c) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[9].Identity, AsByteView(Generated::Lambert_262c37a4) },
			ShaderPatchEntry{ FixId::LambertDecodeGuard, &LambertShaderIdentities[10].Identity, AsByteView(Generated::Lambert_e73d9a26) },
		};
	}

	auto EmbeddedShaderPatchRegistry::Entries() const noexcept -> std::span<const ShaderPatchEntry>
	{
		return kEntries;
	}
}
