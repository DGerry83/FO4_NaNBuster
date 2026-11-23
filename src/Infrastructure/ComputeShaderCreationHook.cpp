#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/ComputeShaderCreationHook.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" alignas(64) void* NaNBuster_ComputeHookState[3] = { nullptr, nullptr, nullptr };

extern "C" void NaNBuster_ComputeDispatchShaderCreation(
	const std::byte* a_data,
	std::size_t a_size,
	NaNBuster::HookCreationContext* a_context) noexcept
{
	auto* hook = static_cast<NaNBuster::ComputeShaderCreationHook*>(NaNBuster_ComputeHookState[2]);
	if (hook == nullptr) {
		return;
	}
	static_cast<void>(hook->Dispatch(NaNBuster::ByteView{ a_data, a_size }, *a_context));
}

namespace NaNBuster
{
	namespace
	{
		constexpr std::array<std::byte, 41> kExpectedSiteBytes = {
			std::byte{ 0x48 }, std::byte{ 0x8b }, std::byte{ 0x0d }, std::byte{ 0xa7 }, std::byte{ 0xae }, std::byte{ 0x38 }, std::byte{ 0x04 },
			std::byte{ 0x44 }, std::byte{ 0x8b }, std::byte{ 0x44 }, std::byte{ 0x24 }, std::byte{ 0x58 },
			std::byte{ 0x48 }, std::byte{ 0x8d }, std::byte{ 0x43 }, std::byte{ 0x08 },
			std::byte{ 0x4c }, std::byte{ 0x8b }, std::byte{ 0x11 },
			std::byte{ 0x48 }, std::byte{ 0x8d }, std::byte{ 0x93 }, std::byte{ 0x88 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
			std::byte{ 0x45 }, std::byte{ 0x33 }, std::byte{ 0xc9 },
			std::byte{ 0x48 }, std::byte{ 0x89 }, std::byte{ 0x44 }, std::byte{ 0x24 }, std::byte{ 0x20 },
			std::byte{ 0x41 }, std::byte{ 0xff }, std::byte{ 0x92 }, std::byte{ 0x90 }, std::byte{ 0x00 }, std::byte{ 0x00 }, std::byte{ 0x00 },
		};

		constexpr std::uint32_t kOwnerRelocationID = 957517;
		constexpr std::ptrdiff_t kSiteOffset = 0x39A;
		constexpr std::size_t kFingerprintSize = kExpectedSiteBytes.size();
		constexpr std::uint32_t kDeviceGlobalRelocationID = 1454788;
		constexpr std::size_t kDetourSize = 7;
		constexpr std::size_t kHookStateElements = 3;

		static_assert(kFingerprintSize == 41);
		static_assert(kHookStateElements == 3);

		constexpr std::array<std::byte, kDetourSize> kOriginalDetourBytes = {
			std::byte{ 0x48 }, std::byte{ 0x8b }, std::byte{ 0x0d }, std::byte{ 0xa7 },
			std::byte{ 0xae }, std::byte{ 0x38 }, std::byte{ 0x04 },
		};
	}

	ComputeShaderCreationHook::ComputeShaderCreationHook(
		const ShaderPatchService& service,
		IEngineShaderCreator& creator,
		OutcomeLogger logOutcome,
		SubstitutionLogger logSubstitution) noexcept :
		m_service(service),
		m_creator(creator),
		m_logOutcome(logOutcome),
		m_logSubstitution(logSubstitution)
	{
	}

	auto ComputeShaderCreationHook::Install() noexcept -> bool
	{
		if (m_attempted) {
			return m_installed;
		}
		m_attempted = true;

		try {
			const REL::Id<> siteId(kOwnerRelocationID);
			const REL::Id<> deviceGlobalId(kDeviceGlobalRelocationID);
			const REL::Relocation<std::uintptr_t> site(siteId, kSiteOffset);
			const auto siteAddress = site.get();
			const REL::Relocation<std::uintptr_t> deviceGlobal(deviceGlobalId);
			const auto deviceGlobalAddress = deviceGlobal.get();
			const auto entryAddress = reinterpret_cast<std::uintptr_t>(&NaNBuster_ComputeHookEntry);
			const auto resumeAddress = siteAddress + kFingerprintSize;
			if (siteAddress == 0 || deviceGlobalAddress == 0) {
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}

			const auto* currentBytes = reinterpret_cast<const std::byte*>(siteAddress);
			if (std::memcmp(currentBytes, kExpectedSiteBytes.data(), kFingerprintSize) != 0) {
				m_logOutcome(HookOutcome::HookUnavailable);
				return false;
			}

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

			NaNBuster_ComputeHookState[0] = std::bit_cast<void*>(deviceGlobalAddress);
			NaNBuster_ComputeHookState[1] = std::bit_cast<void*>(resumeAddress);
			NaNBuster_ComputeHookState[2] = const_cast<ComputeShaderCreationHook*>(this);

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
				NaNBuster_ComputeHookState[0] = nullptr;
				NaNBuster_ComputeHookState[1] = nullptr;
				NaNBuster_ComputeHookState[2] = nullptr;
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
			m_logOutcome(HookOutcome::HookUnavailable);
			return false;
		}
	}

	auto ComputeShaderCreationHook::IsInstalled() const noexcept -> bool
	{
		return m_installed;
	}

	auto ComputeShaderCreationHook::Dispatch(ByteView originalBytes, const HookCreationContext& context) noexcept -> EngineCreationOutcome
	{
		const ShaderSelection selection = m_service.Select(originalBytes);
		if (!selection.Substitute) {
			return m_creator.Create(selection.SelectedBytecode, const_cast<OgCreationContext*>(&context.engine));
		}

		const EngineCreationOutcome corrected = m_creator.Create(
			selection.SelectedBytecode, const_cast<OgCreationContext*>(&context.engine));
		if (corrected.Succeeded()) {
			if (m_logSubstitution != nullptr && selection.SourceIdentity != nullptr) {
				m_logSubstitution(*selection.SourceIdentity);
			} else {
				m_logOutcome(HookOutcome::TargetSubstituted);
			}
			return corrected;
		}

		m_logOutcome(HookOutcome::CorrectedCreationFailedOriginal);
		return m_creator.Create(originalBytes, const_cast<OgCreationContext*>(&context.engine));
	}

	auto ComputeShaderReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext& context) noexcept -> std::intptr_t
	{
		const auto* hookContext = reinterpret_cast<const HookCreationContext*>(&context);
		return NaNBuster_ComputeReplayOriginalCreation(selectedBytecode.data(), selectedBytecode.size(), *hookContext);
	}
}
