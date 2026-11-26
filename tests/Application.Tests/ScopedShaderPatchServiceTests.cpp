#include "Application/ShaderPatchRegistry.h"
#include "Application/ShaderPatchService.h"
#include "Core/LambertShaderIdentities.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_CORRECTED_SHADER_PATH
#	error "NANBUSTER_CORRECTED_SHADER_PATH is required."
#endif
#ifndef NANBUSTER_LAMBERT_REFERENCE_ROOT
#	error "NANBUSTER_LAMBERT_REFERENCE_ROOT is required."
#endif
#ifndef NANBUSTER_ISOTROPIC_ORIGINAL_PATH
#	error "NANBUSTER_ISOTROPIC_ORIGINAL_PATH is required."
#endif

namespace
{
	class TestRegistry final : public NaNBuster::IShaderPatchRegistry
	{
	public:
		TestRegistry()
		{
			const std::filesystem::path root = NANBUSTER_LAMBERT_REFERENCE_ROOT;
			m_originals.push_back(NaNBuster::TestFixtures::LoadOriginalShader());
			for (const auto& identity : NaNBuster::LambertShaderIdentities) {
				m_originals.push_back(NaNBuster::TestFixtures::LoadBinaryFile(
					(root / identity.ReferenceStem / "original.dxbc").string()));
			}

			Add(0, NaNBuster::FixId::Color0LogClamp, &NaNBuster::OriginalTargetIdentity);
			Add(2, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[1].Identity);
			Add(2, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[1].Identity);
			Add(12, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[11].Identity);
			Add(12, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[11].Identity);
			Add(3, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[2].Identity);
			Add(3, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[2].Identity);
			Add(4, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[3].Identity);
			Add(4, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[3].Identity);
			Add(6, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[5].Identity);
			Add(6, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[5].Identity);
			Add(8, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[7].Identity);
			Add(8, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[7].Identity);
			Add(10, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[9].Identity);
			Add(10, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[9].Identity);
			for (std::size_t i = 1; i < m_originals.size(); ++i) {
				Add(i, NaNBuster::FixId::LambertDecodeGuard, &NaNBuster::LambertShaderIdentities[i - 1].Identity);
			}
			m_originals.push_back(NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH));
			Add(13, NaNBuster::FixId::IsotropicSpecularAA, &NaNBuster::IsotropicSpecularTargetIdentity);
		}

		auto Entries() const noexcept -> std::span<const NaNBuster::ShaderPatchEntry> override
		{
			return m_entries;
		}

		auto Original(std::size_t index) const -> NaNBuster::ByteView
		{
			return NaNBuster::TestFixtures::AsByteView(m_originals[index]);
		}

		auto Entry(std::size_t originalIndex, NaNBuster::FixId fix) const -> std::size_t
		{
			for (std::size_t i = 0; i < m_entries.size(); ++i) {
				if (m_entryOriginals[i] == originalIndex && m_entries[i].Fix == fix) {
					return i;
				}
			}
			std::abort();
		}

		auto Corrected(std::size_t entryIndex) const -> const std::byte*
		{
			return m_corrected.data() + entryIndex;
		}

	private:
		void Add(std::size_t originalIndex, NaNBuster::FixId fix, const NaNBuster::ShaderIdentity* identity)
		{
			const std::size_t entryIndex = m_entryCount++;
			m_entryOriginals[entryIndex] = originalIndex;
			m_entries[entryIndex] = NaNBuster::ShaderPatchEntry{
				fix,
				identity,
				NaNBuster::ByteView{ m_corrected.data() + entryIndex, 1 },
			};
		}

		std::vector<std::vector<std::byte>> m_originals;
		std::array<std::byte, 28> m_corrected{};
		std::array<NaNBuster::ShaderPatchEntry, 28> m_entries{};
		std::array<std::size_t, 28> m_entryOriginals{};
		std::size_t m_entryCount = 0;
	};

	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

	void ExpectPassthrough(
		const NaNBuster::ShaderPatchService& service,
		NaNBuster::ByteView input)
	{
		const auto selection = service.Select(input);
		CHECK(!selection.Substitute);
		CHECK(selection.Outcome == NaNBuster::PatchOutcome::NotTarget);
		CHECK(selection.SelectedBytecode.data() == input.data());
		CHECK(selection.SelectedBytecode.size() == input.size());
	}

	void ExpectSelection(
		const TestRegistry& registry,
		const NaNBuster::ShaderPatchService& service,
		std::size_t originalIndex,
		NaNBuster::FixId fix)
	{
		const auto selection = service.Select(registry.Original(originalIndex));
		CHECK(selection.Substitute);
		CHECK(selection.Outcome == NaNBuster::PatchOutcome::Patched);
		CHECK(selection.SelectedBytecode.data() == registry.Corrected(registry.Entry(originalIndex, fix)));
		CHECK(selection.SelectedBytecode.size() == 1);
	}
}

auto main() -> int
{
	const TestRegistry registry;
	const NaNBuster::ShaderPatchService allEnabled(registry, NaNBuster::FixPolicy::AllEnabled());
	for (std::size_t i = 0; i < 14; ++i) {
		const auto fix = i == 0 ? NaNBuster::FixId::Color0LogClamp :
			i == 13 ? NaNBuster::FixId::IsotropicSpecularAA :
			(i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12) ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp :
				NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, allEnabled, i, fix);
	}

	const NaNBuster::ShaderPatchService lambertOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = false, .AnisotropicSpecularClampEnabled = true });
	ExpectSelection(registry, lambertOff, 0, NaNBuster::FixId::Color0LogClamp);
	for (const auto originalIndex : { 2, 3, 4, 6, 8, 10, 12 }) {
		ExpectSelection(registry, lambertOff, originalIndex, NaNBuster::FixId::AnisotropicSpecularClamp);
	}
	for (std::size_t i = 1; i < 13; ++i) {
		if (i != 2 && i != 3 && i != 4 && i != 6 && i != 8 && i != 10 && i != 12) {
			ExpectPassthrough(lambertOff, registry.Original(i));
		}
	}
	ExpectSelection(registry, lambertOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService anisotropicOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = true, .AnisotropicSpecularClampEnabled = false });
	ExpectSelection(registry, anisotropicOff, 0, NaNBuster::FixId::Color0LogClamp);
	for (std::size_t i = 1; i < 13; ++i) {
		ExpectSelection(registry, anisotropicOff, i, NaNBuster::FixId::LambertDecodeGuard);
	}
	ExpectSelection(registry, anisotropicOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService colorOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = true, .AnisotropicSpecularClampEnabled = true });
	ExpectPassthrough(colorOff, registry.Original(0));
	for (std::size_t i = 1; i < 13; ++i) {
		const auto fix = (i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12) ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp : NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, colorOff, i, fix);
	}
	ExpectSelection(registry, colorOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService bothOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = false, .AnisotropicSpecularClampEnabled = false });
	for (std::size_t i = 0; i < 13; ++i) {
		ExpectPassthrough(bothOff, registry.Original(i));
	}
	ExpectSelection(registry, bothOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService isotropicOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = true, .AnisotropicSpecularClampEnabled = true, .IsotropicSpecularAAEnabled = false });
	ExpectPassthrough(isotropicOff, registry.Original(13));
	for (std::size_t i = 0; i < 13; ++i) {
		const auto fix = i == 0 ? NaNBuster::FixId::Color0LogClamp :
			(i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12) ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp :
				NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, isotropicOff, i, fix);
	}

	const NaNBuster::ShaderPatchService allOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = false, .AnisotropicSpecularClampEnabled = false, .IsotropicSpecularAAEnabled = false });
	for (std::size_t i = 0; i < 14; ++i) {
		ExpectPassthrough(allOff, registry.Original(i));
	}

	const auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(corrected));
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{});
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{ corrected.data(), 19 });

	auto flipped = NaNBuster::TestFixtures::LoadOriginalShader();
	flipped[4] ^= std::byte{ 0xFF };
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flipped));
	auto flippedIsotropic = NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH);
	flippedIsotropic[4] ^= std::byte{ 0xFF };
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flippedIsotropic));

	if (g_failures != 0) {
		std::cerr << "Application.ScopedShaderPatchService: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Application.ScopedShaderPatchService: all checks passed (14 identities, 28 entries, independent toggles, passthrough).\n";
	return EXIT_SUCCESS;
}
