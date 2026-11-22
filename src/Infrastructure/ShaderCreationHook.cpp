// CommonLib first: its headers are not standalone and require the umbrella
// include ahead of any Windows API header (the hook header pulls in d3d11.h).
// The pinned CommonLib's own templates (REX/Crc.hpp via REL::Id, REL/Runtime)
// trip C4267/C4244 under the consumer warning policy; scoped to this TU.
#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/ShaderCreationHook.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Process-lifetime native hook state, read by the MASM entry/replay on every
// intercepted creation. Written once by Install() immediately before the
// detour is published (the detour write is the release that makes the engine
// path see it) and never mutated afterwards. This is the documented
// intentional process-global state of the C12 native checklist; element
// indices are mirrored by the NTS_* EQU checks in ShaderCreationHook.asm.
extern "C" alignas(64) void* NaNBuster_HookState[3] = { nullptr, nullptr, nullptr };

// Native entry thunk called by the MASM detour entry. Win64 scalar-only ABI:
// rcx = original bytes, rdx = original size, r8 = &HookCreationContext, void
// return. The native entry already resumes the engine with rax = engine
// wrapper, so the dispatch outcome is deliberately discarded and no 16-byte
// aggregate (with a hidden return pointer shifting the argument registers)
// ever enters the native contract.
extern "C" void NaNBuster_DispatchShaderCreation(
	const std::byte* a_data,
	std::size_t a_size,
	NaNBuster::HookCreationContext* a_context) noexcept
{
	auto* hook = static_cast<NaNBuster::ShaderCreationHook*>(NaNBuster_HookState[2]);
	if (hook == nullptr) {
		return; // unreachable after a valid install; creates no outcome object
	}
	static_cast<void>(hook->Dispatch(NaNBuster::ByteView{ a_data, a_size }, *a_context));
}

namespace NaNBuster
{
	namespace
	{
		// G5_HOOK_CONTRACT.md §4 accepted fingerprint: the complete 34 bytes at
		// 0x141d10e82 (two independent reads agree). Verified verbatim before
		// any engine write; the first 7 bytes are also the restore image if a
		// post-write verification ever fails.
		constexpr std::array<std::byte, 34> kExpectedSiteBytes = {
			std::byte{ 0x48 }, std::byte{ 0x8b }, std::byte{ 0x0d }, std::byte{ 0xff }, std::byte{ 0xb0 }, std::byte{ 0x38 }, std::byte{ 0x04 },
			std::byte{ 0x44 }, std::byte{ 0x8b }, std::byte{ 0x44 }, std::byte{ 0x24 }, std::byte{ 0x60 },
			std::byte{ 0x48 }, std::byte{ 0x8d }, std::byte{ 0x47 }, std::byte{ 0x08 },
			std::byte{ 0x4c }, std::byte{ 0x8b }, std::byte{ 0x11 },
			std::byte{ 0x45 }, std::byte{ 0x33 }, std::byte{ 0xc9 },
			std::byte{ 0x48 }, std::byte{ 0x8b }, std::byte{ 0xd3 },
			std::byte{ 0x48 }, std::byte{ 0x89 }, std::byte{ 0x44 }, std::byte{ 0x24 }, std::byte{ 0x20 },
			std::byte{ 0x41 }, std::byte{ 0xff }, std::byte{ 0x52 }, std::byte{ 0x78 },
		};

		// Address-Library identities (G5 §2). The site is resolved through the
		// owner function relocation plus the accepted image-relative offset;
		// no runtime VA is hardcoded.
		constexpr std::uint32_t kOwnerRelocationID = 957517;   // CreatePixelShaderFromStream
		constexpr std::ptrdiff_t kSiteOffset = 0x142;
		constexpr std::size_t kFingerprintSize = kExpectedSiteBytes.size();
		constexpr std::uint32_t kDeviceGlobalRelocationID = 1454788; // ID3D11Device* global
		constexpr std::size_t kDetourSize = 7;                 // E9 rel32 + NOP + NOP
		constexpr std::size_t kHookStateElements = 3;          // mirrored by the .asm NTS_* checks

		static_assert(kFingerprintSize == 34);
		static_assert(kHookStateElements == 3);

		// The original first 7 bytes of the accepted site, used as the restore
		// image if the detour write or its verification ever fails.
		constexpr std::array<std::byte, kDetourSize> kOriginalDetourBytes = {
			std::byte{ 0x48 }, std::byte{ 0x8b }, std::byte{ 0x0d }, std::byte{ 0xff },
			std::byte{ 0xb0 }, std::byte{ 0x38 }, std::byte{ 0x04 },
		};
	}

	ShaderCreationHook::ShaderCreationHook(
		const ShaderPatchService& service,
		IEngineShaderCreator& creator,
		OutcomeLogger logOutcome) noexcept :
		m_service(service),
		m_creator(creator),
		m_logOutcome(logOutcome)
	{
	}

	auto ShaderCreationHook::Install() noexcept -> bool
	{
		if (m_attempted) {
			return m_installed;
		}
		m_attempted = true;

		try {
			// Address resolution failure is reported as address 0 by REL::Id;
			// treat it (and any exception from the database) as unavailable.
			const REL::Id<> siteId(kOwnerRelocationID);
			const REL::Id<> deviceGlobalId(kDeviceGlobalRelocationID);
			const REL::Relocation<std::uintptr_t> site(siteId, kSiteOffset);
			const auto siteAddress = site.get();
			const REL::Relocation<std::uintptr_t> deviceGlobal(deviceGlobalId);
			const auto deviceGlobalAddress = deviceGlobal.get();
			const auto entryAddress = reinterpret_cast<std::uintptr_t>(&NaNBuster_HookEntry);
			const auto resumeAddress = siteAddress + kFingerprintSize;
			if (siteAddress == 0 || deviceGlobalAddress == 0) {
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}

			// Fingerprint gate: the engine is untouched unless all 34 accepted
			// bytes are present.
			const auto* currentBytes = reinterpret_cast<const std::byte*>(siteAddress);
			if (std::memcmp(currentBytes, kExpectedSiteBytes.data(), kFingerprintSize) != 0) {
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}

			// Near-branch stub in the F4SE branch pool (within rel32 range of
			// the game image): FF 25 + rip-disp0 + absolute VA of the native
			// entry. The hook DLL can sit farther than +/-2 GB from the game,
			// so the 5-byte engine JMP targets this stub, never the DLL directly.
			constexpr std::size_t kStubSize = 14;
			auto* stub = F4SE::GetTrampolineInterface()->AllocateFromBranchPool(kStubSize);
			const std::array<std::byte, kStubSize> stubTemplate = {
				std::byte{ 0xff }, std::byte{ 0x25 },
				std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
				std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
				std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
			};
			std::memcpy(stub, stubTemplate.data(), kStubSize);
			*reinterpret_cast<std::uintptr_t*>(stub + 6) = entryAddress;

			const auto rel32 = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(stub)) -
				static_cast<std::int64_t>(siteAddress + 5);
			if (rel32 < INT32_MIN || rel32 > INT32_MAX) {
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}
			const auto rel = static_cast<std::uint32_t>(static_cast<std::uint32_t>(rel32) & 0xffffffffu);
			const std::array<std::byte, kDetourSize> detour = {
				std::byte{ 0xe9 },
				std::byte{ static_cast<std::uint8_t>(rel & 0xffu) },
				std::byte{ static_cast<std::uint8_t>((rel >> 8) & 0xffu) },
				std::byte{ static_cast<std::uint8_t>((rel >> 16) & 0xffu) },
				std::byte{ static_cast<std::uint8_t>((rel >> 24) & 0xffu) },
				std::byte{ 0x90 },
				std::byte{ 0x90 },
			};

			// Publish native state, then release the detour with a checked safe
			// write (VirtualProtect -> copy -> restore protection -> flush, with
			// an inspectable REX::SystemError at every step). Until the write
			// lands, the engine cannot reach the entry, so the state is
			// invisible. If the write or its verification fails, the original 7
			// bytes are restored with a second checked WriteSafe and the engine
			// bytes are re-read; the D9 truth table
			// (ClassifyDetourWriteRecovery) then decides:
			//   RestoredOriginal -> clear native state (no detour exists),
			//      report HookUnavailable once, return false.
			//   DetourIntact     -> the complete intended detour is present, so
			//      the engine path reaches the native entry and REQUIRES live
			//      dispatch state; keep it, report PatchInstalled, return true.
			//   Irrecoverable    -> bytes are neither original nor detour;
			//      never clear dispatch state (a partially intact JMP could
			//      still reach the entry) and never claim restoration; report
			//      HookUnavailable once, return false. Fingerprint failure
			//      above still writes nothing.
			NaNBuster_HookState[0] = std::bit_cast<void*>(deviceGlobalAddress);
			NaNBuster_HookState[1] = std::bit_cast<void*>(resumeAddress);
			NaNBuster_HookState[2] = const_cast<ShaderCreationHook*>(this);

			const REX::SystemError detourError = REL::WriteSafe(siteAddress, detour.data(), detour.size());
			std::array<std::byte, kDetourSize> resulting{};
			std::memcpy(resulting.data(), currentBytes, kDetourSize);
			if (!detourError && resulting == detour) {
				m_installed = true;
				m_logOutcome(HookOutcome::PatchInstalled);
				return true;
			}

			const REX::SystemError restoreError = REL::WriteSafe(
				siteAddress, kExpectedSiteBytes.data(), kDetourSize);
			std::array<std::byte, kDetourSize> currentAfterRestore{};
			std::memcpy(currentAfterRestore.data(), currentBytes, kDetourSize);

			switch (ClassifyDetourWriteRecovery(
				restoreError ? true : false,
				currentAfterRestore == kOriginalDetourBytes,
				currentAfterRestore == detour)) {
			case DetourWriteRecovery::RestoredOriginal:
				NaNBuster_HookState[0] = nullptr;
				NaNBuster_HookState[1] = nullptr;
				NaNBuster_HookState[2] = nullptr;
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			case DetourWriteRecovery::DetourIntact:
				m_installed = true;
				m_logOutcome(HookOutcome::PatchInstalled);
				return true;
			case DetourWriteRecovery::Irrecoverable:
			default:
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}
		} catch (...) {
			// Resolution/lookup/allocation failure: engine unchanged (no write
			// happened), report once.
			m_logOutcome(HookOutcome::HookUnavailable);
			return false;
		}
	}

	auto ShaderCreationHook::IsInstalled() const noexcept -> bool
	{
		return m_installed;
	}

	auto ShaderCreationHook::Dispatch(ByteView originalBytes, const HookCreationContext& context) noexcept -> EngineCreationOutcome
	{
		// C8 guarantees: nonmatch -> caller's original view, Substitute false,
		// payload never queried; exact match -> payload corrected view.
		const ShaderSelection selection = m_service.Select(originalBytes);
		if (!selection.Substitute) {
			return m_creator.Create(selection.SelectedBytecode, const_cast<OgCreationContext*>(&context.engine));
		}

		const EngineCreationOutcome corrected = m_creator.Create(
			selection.SelectedBytecode, const_cast<OgCreationContext*>(&context.engine));
		if (corrected.Succeeded()) {
			m_logOutcome(HookOutcome::TargetSubstituted);
			return corrected;
		}

		m_logOutcome(HookOutcome::CorrectedCreationFailedOriginal);
		return m_creator.Create(originalBytes, const_cast<OgCreationContext*>(&context.engine));
	}

	auto HookReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext& context) noexcept -> std::intptr_t
	{
		// C11 hands us the address of the OgCreationContext member of the
		// HookCreationContext that Dispatch received; the member sits at offset
		// 0 (static-asserted in the header), so the outer wrapper — and its
		// captured site stack — is recoverable here.
		const auto* hookContext = reinterpret_cast<const HookCreationContext*>(&context);
		return NaNBuster_ReplayOriginalCreation(selectedBytecode.data(), selectedBytecode.size(), *hookContext);
	}
}
