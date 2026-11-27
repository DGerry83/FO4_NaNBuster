#pragma once

#include "Infrastructure/NativeHookDefinition.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace NaNBuster
{
enum class DetourWriteRecovery
{
	RestoredOriginal,
	DetourIntact,
	Irrecoverable,
};

[[nodiscard]] constexpr auto ClassifyDetourWriteRecovery(bool a_restoreFailed, bool a_bytesMatchOriginal,
														 bool a_bytesMatchDetour) noexcept -> DetourWriteRecovery
{
	if (!a_restoreFailed && a_bytesMatchOriginal)
	{
		return DetourWriteRecovery::RestoredOriginal;
	}
	if (a_bytesMatchDetour)
	{
		return DetourWriteRecovery::DetourIntact;
	}
	return DetourWriteRecovery::Irrecoverable;
}

enum class NativeHookInstallResult
{
	Unavailable,
	Installed,
	DetourIntact,
};

struct NativeHookOperations
{
	std::uintptr_t (*ResolveAddress)(std::uint32_t a_id, std::ptrdiff_t a_offset);
	void *(*AllocateBranchStub)(std::size_t a_size);
	bool (*WriteSafe)(std::uintptr_t a_address, const std::byte *a_data, std::size_t a_size);
};

struct NativeHookInstallRequest
{
	const NativeHookDefinition *Definition;
	std::uintptr_t EntryAddress;
	void *HookInstance;
	std::span<void *, 3> HookState;
	const NativeHookOperations *Operations;
};

[[nodiscard]] auto DefaultNativeHookOperations() noexcept -> NativeHookOperations;
[[nodiscard]] auto InstallNativeHook(const NativeHookInstallRequest &a_request) noexcept -> NativeHookInstallResult;
} // namespace NaNBuster
