#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/NativeHookInstaller.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>

namespace NaNBuster
{
namespace
{
constexpr std::size_t kDetourSize = 7;
constexpr std::size_t kStubSize = 14;

auto ResolveAddress(std::uint32_t a_id, std::ptrdiff_t a_offset) -> std::uintptr_t
{
	const REL::Id<> id(a_id);
	const REL::Relocation<std::uintptr_t> relocation(id, a_offset);
	return relocation.get();
}

auto AllocateBranchStub(std::size_t a_size) -> void *
{
	return F4SE::GetTrampolineInterface()->AllocateFromBranchPool(a_size);
}

auto WriteSafe(std::uintptr_t a_address, const std::byte *a_data, std::size_t a_size) -> bool
{
	return !REL::WriteSafe(a_address, a_data, a_size);
}

auto BytesEqual(const std::byte *a_lhs, const std::byte *a_rhs, std::size_t a_size) -> bool
{
	return std::memcmp(a_lhs, a_rhs, a_size) == 0;
}
} // namespace

auto DefaultNativeHookOperations() noexcept -> NativeHookOperations
{
	return NativeHookOperations{
		.ResolveAddress = &ResolveAddress,
		.AllocateBranchStub = &AllocateBranchStub,
		.WriteSafe = &WriteSafe,
	};
}

auto InstallNativeHook(const NativeHookInstallRequest &a_request) noexcept -> NativeHookInstallResult
{
	try
	{
		const auto *definition = a_request.Definition;
		const auto *operations = a_request.Operations;
		if (definition == nullptr || operations == nullptr || operations->ResolveAddress == nullptr ||
			operations->AllocateBranchStub == nullptr || operations->WriteSafe == nullptr ||
			a_request.EntryAddress == 0 || a_request.HookInstance == nullptr || definition->DetourSize != kDetourSize ||
			definition->ExpectedSiteBytes.size() < kDetourSize)
		{
			return NativeHookInstallResult::Unavailable;
		}

		const auto siteAddress = operations->ResolveAddress(definition->OwnerRelocationID, definition->SiteOffset);
		const auto deviceGlobalAddress = operations->ResolveAddress(definition->DeviceGlobalRelocationID, 0);
		if (siteAddress == 0 || deviceGlobalAddress == 0)
		{
			return NativeHookInstallResult::Unavailable;
		}

		const auto *currentBytes = reinterpret_cast<const std::byte *>(siteAddress);
		if (!BytesEqual(currentBytes, definition->ExpectedSiteBytes.data(), definition->ExpectedSiteBytes.size()))
		{
			return NativeHookInstallResult::Unavailable;
		}

		auto *stub = static_cast<std::byte *>(operations->AllocateBranchStub(kStubSize));
		if (stub == nullptr)
		{
			return NativeHookInstallResult::Unavailable;
		}
		constexpr std::array<std::byte, kStubSize> kStubTemplate = {
			std::byte{0xff}, std::byte{0x25}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
			std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
			std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
		};
		std::memcpy(stub, kStubTemplate.data(), kStubSize);
		const auto entryAddress = a_request.EntryAddress;
		std::memcpy(stub + 6, &entryAddress, sizeof(entryAddress));

		const auto rel32 = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(stub)) -
						   static_cast<std::int64_t>(siteAddress + 5);
		if (rel32 < std::numeric_limits<std::int32_t>::min() || rel32 > std::numeric_limits<std::int32_t>::max())
		{
			return NativeHookInstallResult::Unavailable;
		}
		const auto rel = static_cast<std::uint32_t>(static_cast<std::uint32_t>(rel32) & 0xffffffffu);
		const std::array<std::byte, kDetourSize> detour = {
			std::byte{0xe9},
			std::byte{static_cast<std::uint8_t>(rel & 0xffu)},
			std::byte{static_cast<std::uint8_t>((rel >> 8) & 0xffu)},
			std::byte{static_cast<std::uint8_t>((rel >> 16) & 0xffu)},
			std::byte{static_cast<std::uint8_t>((rel >> 24) & 0xffu)},
			std::byte{0x90},
			std::byte{0x90},
		};

		a_request.HookState[0] = std::bit_cast<void *>(deviceGlobalAddress);
		a_request.HookState[1] = std::bit_cast<void *>(siteAddress + definition->ExpectedSiteBytes.size());
		a_request.HookState[2] = a_request.HookInstance;

		const bool detourWritten = operations->WriteSafe(siteAddress, detour.data(), detour.size());
		std::array<std::byte, kDetourSize> resulting{};
		std::memcpy(resulting.data(), currentBytes, kDetourSize);
		if (detourWritten && BytesEqual(resulting.data(), detour.data(), kDetourSize))
		{
			return NativeHookInstallResult::Installed;
		}

		const bool restoreWritten =
			operations->WriteSafe(siteAddress, definition->ExpectedSiteBytes.data(), kDetourSize);
		std::array<std::byte, kDetourSize> currentAfterRestore{};
		std::memcpy(currentAfterRestore.data(), currentBytes, kDetourSize);

		const auto recovery = ClassifyDetourWriteRecovery(
			!restoreWritten, BytesEqual(currentAfterRestore.data(), definition->ExpectedSiteBytes.data(), kDetourSize),
			BytesEqual(currentAfterRestore.data(), detour.data(), kDetourSize));
		switch (recovery)
		{
		case DetourWriteRecovery::RestoredOriginal:
			a_request.HookState[0] = nullptr;
			a_request.HookState[1] = nullptr;
			a_request.HookState[2] = nullptr;
			return NativeHookInstallResult::Unavailable;
		case DetourWriteRecovery::DetourIntact:
			return NativeHookInstallResult::DetourIntact;
		case DetourWriteRecovery::Irrecoverable:
		default:
			return NativeHookInstallResult::Unavailable;
		}
	}
	catch (...)
	{
		return NativeHookInstallResult::Unavailable;
	}
}
} // namespace NaNBuster
