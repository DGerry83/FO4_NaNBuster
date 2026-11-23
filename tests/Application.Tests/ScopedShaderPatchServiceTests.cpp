#include "Application/ShaderPatchRegistry.h"
#include "Application/ShaderPatchService.h"
#include "Core/LambertShaderIdentities.h"
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
			for (std::size_t i = 0; i < m_originals.size(); ++i) {
				const auto* identity = i == 0 ? &NaNBuster::OriginalTargetIdentity : &NaNBuster::LambertShaderIdentities[i - 1].Identity;
				const auto fix = i == 0 ? NaNBuster::FixId::Color0LogClamp : NaNBuster::FixId::LambertDecodeGuard;
				m_entries[i] = NaNBuster::ShaderPatchEntry{
					fix,
					identity,
					NaNBuster::ByteView{ m_corrected.data() + i, 1 },
				};
			}
		}

		auto Entries() const noexcept -> std::span<const NaNBuster::ShaderPatchEntry> override
		{
			return m_entries;
		}

		auto Original(std::size_t index) const -> NaNBuster::ByteView
		{
			return NaNBuster::TestFixtures::AsByteView(m_originals[index]);
		}

		auto Corrected(std::size_t index) const -> const std::byte*
		{
			return m_corrected.data() + index;
		}

	private:
		std::vector<std::vector<std::byte>> m_originals;
		std::array<std::byte, 12> m_corrected{};
		std::array<NaNBuster::ShaderPatchEntry, 12> m_entries{};
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
}

auto main() -> int
{
	const TestRegistry registry;
	const NaNBuster::ShaderPatchService allEnabled(registry, NaNBuster::FixPolicy::AllEnabled());
	for (std::size_t i = 0; i < 12; ++i) {
		const auto selection = allEnabled.Select(registry.Original(i));
		CHECK(selection.Substitute);
		CHECK(selection.Outcome == NaNBuster::PatchOutcome::Patched);
		CHECK(selection.SelectedBytecode.data() == registry.Corrected(i));
		CHECK(selection.SelectedBytecode.size() == 1);
	}

	const NaNBuster::ShaderPatchService lambertOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = true, .LambertDecodeGuardEnabled = false });
	CHECK(lambertOff.Select(registry.Original(0)).Substitute);
	for (std::size_t i = 1; i < 12; ++i) {
		ExpectPassthrough(lambertOff, registry.Original(i));
	}

	const NaNBuster::ShaderPatchService colorOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = true });
	ExpectPassthrough(colorOff, registry.Original(0));
	for (std::size_t i = 1; i < 12; ++i) {
		CHECK(colorOff.Select(registry.Original(i)).Substitute);
	}

	const NaNBuster::ShaderPatchService bothOff(
		registry,
		NaNBuster::FixPolicy{ .Color0LogClampEnabled = false, .LambertDecodeGuardEnabled = false });
	for (std::size_t i = 0; i < 12; ++i) {
		ExpectPassthrough(bothOff, registry.Original(i));
	}

	const auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(corrected));
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{});
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{ corrected.data(), 19 });

	auto flipped = NaNBuster::TestFixtures::LoadOriginalShader();
	flipped[4] ^= std::byte{ 0xFF };
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flipped));

	if (g_failures != 0) {
		std::cerr << "Application.ScopedShaderPatchService: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Application.ScopedShaderPatchService: all checks passed (12 matches, independent toggles, passthrough).\n";
	return EXIT_SUCCESS;
}
