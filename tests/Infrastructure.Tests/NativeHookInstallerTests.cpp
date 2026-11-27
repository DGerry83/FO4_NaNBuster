#include "Infrastructure/NativeHookInstaller.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
int g_failures = 0;

void Check(bool a_condition, const char *a_expression, const char *a_file, int a_line)
{
	if (!a_condition)
	{
		++g_failures;
		std::cerr << "FAILED: " << a_expression << " (" << a_file << ':' << a_line << ")\n";
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

constexpr std::array kFingerprint = {
	std::byte{0x48}, std::byte{0x8b}, std::byte{0x0d}, std::byte{0x11}, std::byte{0x22}, std::byte{0x33},
	std::byte{0x44}, std::byte{0x45}, std::byte{0x33}, std::byte{0xc9}, std::byte{0x48}, std::byte{0x8b},
	std::byte{0xd3}, std::byte{0x4c}, std::byte{0x8b}, std::byte{0x11},
};

struct Fixture
{
	Fixture()
	{
		std::copy(kFingerprint.begin(), kFingerprint.end(), Site.begin());
		ApplyWrite.fill(true);
		Operations = NaNBuster::NativeHookOperations{
			.ResolveAddress = &Resolve,
			.AllocateBranchStub = &Allocate,
			.WriteSafe = &Write,
		};
		Definition = NaNBuster::NativeHookDefinition{
			.OwnerRelocationID = 11,
			.SiteOffset = 0x20,
			.DeviceGlobalRelocationID = 22,
			.ExpectedSiteBytes = kFingerprint,
			.DetourSize = 7,
		};
	}

	static auto Resolve(std::uint32_t a_id, std::ptrdiff_t a_offset) -> std::uintptr_t
	{
		auto &self = *Current;
		++self.ResolveCalls;
		if (a_id == self.Definition.OwnerRelocationID)
		{
			CHECK(a_offset == self.Definition.SiteOffset);
			return self.ZeroOwner ? 0 : reinterpret_cast<std::uintptr_t>(self.Site.data());
		}
		if (a_id == self.Definition.DeviceGlobalRelocationID)
		{
			CHECK(a_offset == 0);
			return self.ZeroDevice ? 0 : DeviceAddress;
		}
		CHECK(!"unexpected relocation ID");
		return 0;
	}

	static auto Allocate(std::size_t a_size) -> void *
	{
		auto &self = *Current;
		++self.AllocateCalls;
		CHECK(a_size == 14);
		return self.NullAllocation ? nullptr : self.Stub.data();
	}

	static auto Write(std::uintptr_t a_address, const std::byte *a_data, std::size_t a_size) -> bool
	{
		auto &self = *Current;
		const auto index = self.WriteCalls++;
		CHECK(index < self.FailWrite.size());
		CHECK(a_address == reinterpret_cast<std::uintptr_t>(self.Site.data()));
		if (index < self.ApplyWrite.size() && self.ApplyWrite[index])
		{
			std::memcpy(reinterpret_cast<void *>(a_address), a_data, a_size);
		}
		return index < self.FailWrite.size() ? !self.FailWrite[index] : false;
	}

	auto Run() -> NaNBuster::NativeHookInstallResult
	{
		Current = this;
		const NaNBuster::NativeHookInstallRequest request{
			.Definition = NullDefinition ? nullptr : &Definition,
			.EntryAddress = EntryAddress,
			.HookInstance = &HookInstance,
			.HookState = HookState,
			.Operations = &Operations,
		};
		return NaNBuster::InstallNativeHook(request);
	}

	auto StateCleared() const -> bool
	{
		return HookState[0] == nullptr && HookState[1] == nullptr && HookState[2] == nullptr;
	}

	inline static Fixture *Current = nullptr;
	inline static constexpr std::uintptr_t DeviceAddress = 0x45670000;
	inline static constexpr std::uintptr_t EntryAddress = 0x12345678;

	NaNBuster::NativeHookDefinition Definition{};
	bool NullDefinition = false;
	NaNBuster::NativeHookOperations Operations{};
	alignas(64) std::array<std::byte, 256> Site{};
	alignas(64) std::array<std::byte, 64> Stub{};
	void *HookState[3] = {nullptr, nullptr, nullptr};
	int HookInstance = 0;
	std::array<bool, 4> FailWrite{};
	std::array<bool, 4> ApplyWrite{};
	bool ZeroOwner = false;
	bool ZeroDevice = false;
	bool NullAllocation = false;
	int ResolveCalls = 0;
	int AllocateCalls = 0;
	int WriteCalls = 0;
};
} // namespace

auto main() -> int
{
	{
		Fixture fixture;
		const auto result = fixture.Run();
		CHECK(result == NaNBuster::NativeHookInstallResult::Installed);
		CHECK(fixture.ResolveCalls == 2);
		CHECK(fixture.AllocateCalls == 1);
		CHECK(fixture.WriteCalls == 1);
		CHECK(std::bit_cast<std::uintptr_t>(fixture.HookState[0]) == Fixture::DeviceAddress);
		CHECK(std::bit_cast<std::uintptr_t>(fixture.HookState[1]) ==
			  reinterpret_cast<std::uintptr_t>(fixture.Site.data()) + kFingerprint.size());
		CHECK(fixture.HookState[2] == &fixture.HookInstance);
		CHECK(fixture.Site[0] == std::byte{0xe9});
		CHECK(fixture.Site[5] == std::byte{0x90});
		CHECK(fixture.Site[6] == std::byte{0x90});
		CHECK(fixture.Stub[0] == std::byte{0xff});
		CHECK(fixture.Stub[1] == std::byte{0x25});
		std::uintptr_t entry = 0;
		std::memcpy(&entry, fixture.Stub.data() + 6, sizeof(entry));
		CHECK(entry == Fixture::EntryAddress);
	}

	{
		Fixture fixture;
		fixture.NullDefinition = true;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.ResolveCalls == 0);
		CHECK(fixture.AllocateCalls == 0);
		CHECK(fixture.WriteCalls == 0);
		CHECK(fixture.StateCleared());
	}

	{
		Fixture fixture;
		fixture.ZeroOwner = true;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.AllocateCalls == 0);
		CHECK(fixture.WriteCalls == 0);
		CHECK(fixture.StateCleared());
	}

	{
		Fixture fixture;
		fixture.ZeroDevice = true;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.AllocateCalls == 0);
		CHECK(fixture.WriteCalls == 0);
		CHECK(fixture.StateCleared());
	}

	{
		Fixture fixture;
		fixture.Site[3] ^= std::byte{0xff};
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.AllocateCalls == 0);
		CHECK(fixture.WriteCalls == 0);
		CHECK(fixture.StateCleared());
	}

	{
		Fixture fixture;
		fixture.NullAllocation = true;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.AllocateCalls == 1);
		CHECK(fixture.WriteCalls == 0);
		CHECK(fixture.StateCleared());
	}

	{
		Fixture fixture;
		fixture.FailWrite[0] = true;
		fixture.ApplyWrite[0] = false;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.WriteCalls == 2);
		CHECK(fixture.StateCleared());
		CHECK(std::equal(kFingerprint.begin(), kFingerprint.end(), fixture.Site.begin()));
	}

	{
		Fixture fixture;
		fixture.ApplyWrite[0] = false;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.WriteCalls == 2);
		CHECK(fixture.StateCleared());
		CHECK(std::equal(kFingerprint.begin(), kFingerprint.end(), fixture.Site.begin()));
	}

	{
		Fixture fixture;
		fixture.FailWrite[0] = true;
		fixture.FailWrite[1] = true;
		fixture.ApplyWrite[1] = false;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::DetourIntact);
		CHECK(fixture.WriteCalls == 2);
		CHECK(!fixture.StateCleared());
		CHECK(fixture.HookState[2] == &fixture.HookInstance);
		CHECK(fixture.Site[0] == std::byte{0xe9});
	}

	{
		Fixture fixture;
		fixture.FailWrite[0] = true;
		fixture.ApplyWrite[0] = false;
		fixture.FailWrite[1] = true;
		fixture.ApplyWrite[1] = false;
		CHECK(fixture.Run() == NaNBuster::NativeHookInstallResult::Unavailable);
		CHECK(fixture.WriteCalls == 2);
		CHECK(!fixture.StateCleared());
		CHECK(std::equal(kFingerprint.begin(), kFingerprint.end(), fixture.Site.begin()));
	}

	if (g_failures != 0)
	{
		std::cerr << "Infrastructure.NativeHookInstaller: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.NativeHookInstaller: all checks passed (success, resolution, mismatch, allocation, "
				 "write/restore matrix, native-state safety).\n";
	return EXIT_SUCCESS;
}
