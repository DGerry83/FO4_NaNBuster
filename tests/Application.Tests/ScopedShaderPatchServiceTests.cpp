#include "Application/ShaderPatchRegistry.h"
#include "Application/ShaderPatchService.h"
#include "Core/AEShaderVariantIdentities.h"
#include "Core/IsotropicSpecularShaderIdentity.h"
#include "Core/LambertShaderIdentities.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_CORRECTED_SHADER_PATH
#error "NANBUSTER_CORRECTED_SHADER_PATH is required."
#endif
#ifndef NANBUSTER_LAMBERT_REFERENCE_ROOT
#error "NANBUSTER_LAMBERT_REFERENCE_ROOT is required."
#endif
#ifndef NANBUSTER_ISOTROPIC_ORIGINAL_PATH
#error "NANBUSTER_ISOTROPIC_ORIGINAL_PATH is required."
#endif
#ifndef NANBUSTER_AE_REFERENCE_ROOT
#error "NANBUSTER_AE_REFERENCE_ROOT is required."
#endif
#ifndef NANBUSTER_AE_SHADER_OUTPUT_ROOT
#error "NANBUSTER_AE_SHADER_OUTPUT_ROOT is required."
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
		for (const auto &identity : NaNBuster::LambertShaderIdentities)
		{
			m_originals.push_back(
				NaNBuster::TestFixtures::LoadBinaryFile((root / identity.ReferenceStem / "original.dxbc").string()));
		}

		Add(0, NaNBuster::FixId::Color0LogClamp, &NaNBuster::OriginalTargetIdentity);
		Add(2, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[1].Identity);
		Add(2, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[1].Identity);
		Add(12, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[11].Identity);
		Add(12, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[11].Identity);
		Add(3, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[2].Identity);
		Add(3, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[2].Identity);
		Add(4, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[3].Identity);
		Add(4, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[3].Identity);
		Add(6, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[5].Identity);
		Add(6, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[5].Identity);
		Add(8, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[7].Identity);
		Add(8, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[7].Identity);
		Add(10, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::LambertShaderIdentities[9].Identity);
		Add(10, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::LambertShaderIdentities[9].Identity);
		for (std::size_t i = 1; i < m_originals.size(); ++i)
		{
			Add(i, NaNBuster::FixId::LambertDecodeGuard, &NaNBuster::LambertShaderIdentities[i - 1].Identity);
		}
		m_originals.push_back(NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH));
		Add(13, NaNBuster::FixId::IsotropicSpecularAA, &NaNBuster::IsotropicSpecularTargetIdentity);

		const std::filesystem::path aeRoot = NANBUSTER_AE_REFERENCE_ROOT;
		for (const auto &identity : NaNBuster::AELambertShaderIdentities)
		{
			m_originals.push_back(
				NaNBuster::TestFixtures::LoadBinaryFile((aeRoot / identity.ReferenceStem / "original.dxbc").string()));
		}
		Add(15, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[1].Identity);
		Add(15, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[1].Identity);
		Add(25, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[11].Identity);
		Add(25, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[11].Identity);
		Add(16, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[2].Identity);
		Add(16, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[2].Identity);
		Add(17, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[3].Identity);
		Add(17, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[3].Identity);
		Add(19, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[5].Identity);
		Add(19, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[5].Identity);
		Add(21, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[7].Identity);
		Add(21, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[7].Identity);
		Add(23, NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp,
			&NaNBuster::AELambertShaderIdentities[9].Identity);
		Add(23, NaNBuster::FixId::AnisotropicSpecularClamp, &NaNBuster::AELambertShaderIdentities[9].Identity);
		for (std::size_t i = 14; i < 26; ++i)
		{
			Add(i, NaNBuster::FixId::LambertDecodeGuard, &NaNBuster::AELambertShaderIdentities[i - 14].Identity);
		}
		m_originals.push_back(NaNBuster::TestFixtures::LoadBinaryFile(
			(aeRoot / NaNBuster::AEIsotropicSpecularReferenceStem / "original.dxbc").string()));
		Add(26, NaNBuster::FixId::IsotropicSpecularAA, &NaNBuster::AEIsotropicSpecularTargetIdentity);
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
		for (std::size_t i = 0; i < m_entries.size(); ++i)
		{
			if (m_entryOriginals[i] == originalIndex && m_entries[i].Fix == fix)
			{
				return i;
			}
		}
		std::cerr << "Missing registry mapping: original " << originalIndex << " fix " << static_cast<int>(fix) << '\n';
		std::abort();
	}

	auto Corrected(std::size_t entryIndex) const -> const std::byte *
	{
		return m_corrected.data() + entryIndex;
	}

  private:
	void Add(std::size_t originalIndex, NaNBuster::FixId fix, const NaNBuster::ShaderIdentity *identity)
	{
		const std::size_t entryIndex = m_entryCount++;
		m_entryOriginals[entryIndex] = originalIndex;
		m_entries[entryIndex] = NaNBuster::ShaderPatchEntry{
			fix,
			identity,
			NaNBuster::ByteView{m_corrected.data() + entryIndex, 1},
		};
	}

	std::vector<std::vector<std::byte>> m_originals;
	std::array<std::byte, 55> m_corrected{};
	std::array<NaNBuster::ShaderPatchEntry, 55> m_entries{};
	std::array<std::size_t, 55> m_entryOriginals{};
	std::size_t m_entryCount = 0;
};

int g_failures = 0;

void Check(bool condition, const char *expression, const char *file, int line)
{
	if (!condition)
	{
		++g_failures;
		std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

void ExpectPassthrough(const NaNBuster::ShaderPatchService &service, NaNBuster::ByteView input)
{
	const auto selection = service.Select(input);
	CHECK(!selection.Substitute);
	CHECK(selection.Outcome == NaNBuster::PatchOutcome::NotTarget);
	CHECK(selection.SelectedBytecode.data() == input.data());
	CHECK(selection.SelectedBytecode.size() == input.size());
}

void ExpectSelection(const TestRegistry &registry, const NaNBuster::ShaderPatchService &service,
					 std::size_t originalIndex, NaNBuster::FixId fix)
{
	const auto selection = service.Select(registry.Original(originalIndex));
	if (!selection.Substitute)
	{
		std::cerr << "Selection failed: original " << originalIndex << " fix " << static_cast<int>(fix) << '\n';
	}
	CHECK(selection.Substitute);
	CHECK(selection.Outcome == NaNBuster::PatchOutcome::Patched);
	CHECK(selection.SelectedBytecode.data() == registry.Corrected(registry.Entry(originalIndex, fix)));
	CHECK(selection.SelectedBytecode.size() == 1);
}
} // namespace

auto main() -> int
{
	const TestRegistry registry;
	const NaNBuster::ShaderPatchService allEnabled(registry, NaNBuster::FixPolicy::AllEnabled());
	for (std::size_t i = 0; i < 14; ++i)
	{
		const auto fix = i == 0	   ? NaNBuster::FixId::Color0LogClamp
						 : i == 13 ? NaNBuster::FixId::IsotropicSpecularAA
						 : (i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12)
							 ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp
							 : NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, allEnabled, i, fix);
	}

	const NaNBuster::ShaderPatchService lambertOff(registry,
												   NaNBuster::FixPolicy{.Color0LogClampEnabled = true,
																		.LambertDecodeGuardEnabled = false,
																		.AnisotropicSpecularClampEnabled = true});
	ExpectSelection(registry, lambertOff, 0, NaNBuster::FixId::Color0LogClamp);
	for (const auto originalIndex : {2, 3, 4, 6, 8, 10, 12})
	{
		ExpectSelection(registry, lambertOff, originalIndex, NaNBuster::FixId::AnisotropicSpecularClamp);
	}
	for (std::size_t i = 1; i < 13; ++i)
	{
		if (i != 2 && i != 3 && i != 4 && i != 6 && i != 8 && i != 10 && i != 12)
		{
			ExpectPassthrough(lambertOff, registry.Original(i));
		}
	}
	ExpectSelection(registry, lambertOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService anisotropicOff(registry,
													   NaNBuster::FixPolicy{.Color0LogClampEnabled = true,
																			.LambertDecodeGuardEnabled = true,
																			.AnisotropicSpecularClampEnabled = false});
	ExpectSelection(registry, anisotropicOff, 0, NaNBuster::FixId::Color0LogClamp);
	for (std::size_t i = 1; i < 13; ++i)
	{
		ExpectSelection(registry, anisotropicOff, i, NaNBuster::FixId::LambertDecodeGuard);
	}
	ExpectSelection(registry, anisotropicOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService colorOff(registry,
												 NaNBuster::FixPolicy{.Color0LogClampEnabled = false,
																	  .LambertDecodeGuardEnabled = true,
																	  .AnisotropicSpecularClampEnabled = true});
	ExpectPassthrough(colorOff, registry.Original(0));
	for (std::size_t i = 1; i < 13; ++i)
	{
		const auto fix = (i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12)
							 ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp
							 : NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, colorOff, i, fix);
	}
	ExpectSelection(registry, colorOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService bothOff(registry,
												NaNBuster::FixPolicy{.Color0LogClampEnabled = false,
																	 .LambertDecodeGuardEnabled = false,
																	 .AnisotropicSpecularClampEnabled = false});
	for (std::size_t i = 0; i < 13; ++i)
	{
		ExpectPassthrough(bothOff, registry.Original(i));
	}
	ExpectSelection(registry, bothOff, 13, NaNBuster::FixId::IsotropicSpecularAA);

	const NaNBuster::ShaderPatchService isotropicOff(registry,
													 NaNBuster::FixPolicy{.Color0LogClampEnabled = true,
																		  .LambertDecodeGuardEnabled = true,
																		  .AnisotropicSpecularClampEnabled = true,
																		  .IsotropicSpecularAAEnabled = false});
	ExpectPassthrough(isotropicOff, registry.Original(13));
	for (std::size_t i = 0; i < 13; ++i)
	{
		const auto fix = i == 0 ? NaNBuster::FixId::Color0LogClamp
						 : (i == 2 || i == 3 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12)
							 ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp
							 : NaNBuster::FixId::LambertDecodeGuard;
		ExpectSelection(registry, isotropicOff, i, fix);
	}

	const NaNBuster::ShaderPatchService allOff(registry, NaNBuster::FixPolicy{.Color0LogClampEnabled = false,
																			  .LambertDecodeGuardEnabled = false,
																			  .AnisotropicSpecularClampEnabled = false,
																			  .IsotropicSpecularAAEnabled = false});
	for (std::size_t i = 0; i < 14; ++i)
	{
		ExpectPassthrough(allOff, registry.Original(i));
	}

	for (unsigned mask = 0; mask < 16; ++mask)
	{
		const bool colorEnabled = (mask & 1u) != 0;
		const bool lambertEnabled = (mask & 2u) != 0;
		const bool anisotropicEnabled = (mask & 4u) != 0;
		const bool isotropicEnabled = (mask & 8u) != 0;
		const NaNBuster::ShaderPatchService service(registry, NaNBuster::FixPolicy{
																  .Color0LogClampEnabled = colorEnabled,
																  .LambertDecodeGuardEnabled = lambertEnabled,
																  .AnisotropicSpecularClampEnabled = anisotropicEnabled,
																  .IsotropicSpecularAAEnabled = isotropicEnabled,
															  });
		for (std::size_t originalIndex = 0; originalIndex < 27; ++originalIndex)
		{
			if (originalIndex == 0)
			{
				if (colorEnabled)
				{
					ExpectSelection(registry, service, originalIndex, NaNBuster::FixId::Color0LogClamp);
				}
				else
				{
					ExpectPassthrough(service, registry.Original(originalIndex));
				}
				continue;
			}
			if (originalIndex == 13 || originalIndex == 26)
			{
				if (isotropicEnabled)
				{
					ExpectSelection(registry, service, originalIndex, NaNBuster::FixId::IsotropicSpecularAA);
				}
				else
				{
					ExpectPassthrough(service, registry.Original(originalIndex));
				}
				continue;
			}
			const auto familyIndex = originalIndex >= 14 ? originalIndex - 14 : originalIndex - 1;
			const bool overlap = familyIndex == 1 || familyIndex == 2 || familyIndex == 3 || familyIndex == 5 ||
								 familyIndex == 7 || familyIndex == 9 || familyIndex == 11;
			if (overlap && anisotropicEnabled)
			{
				ExpectSelection(registry, service, originalIndex,
								lambertEnabled ? NaNBuster::FixId::LambertDecodeGuardWithAnisotropicSpecularClamp
											   : NaNBuster::FixId::AnisotropicSpecularClamp);
			}
			else if (lambertEnabled)
			{
				ExpectSelection(registry, service, originalIndex, NaNBuster::FixId::LambertDecodeGuard);
			}
			else
			{
				ExpectPassthrough(service, registry.Original(originalIndex));
			}
		}
	}

	for (std::size_t originalIndex = 0; originalIndex < 27; ++originalIndex)
	{
		const auto input = registry.Original(originalIndex);
		ExpectPassthrough(allEnabled, NaNBuster::ByteView{});
		ExpectPassthrough(allEnabled, NaNBuster::ByteView{input.data(), 19});
		std::vector<std::byte> truncated(input.begin(), input.end() - 1);
		ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(truncated));
		std::vector<std::byte> oversized(input.begin(), input.end());
		oversized.push_back(std::byte{0x00});
		ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(oversized));
		std::vector<std::byte> flipped(input.begin(), input.end());
		flipped[4] ^= std::byte{0xFF};
		ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flipped));
		flipped.assign(input.begin(), input.end());
		flipped[19] ^= std::byte{0xFF};
		ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flipped));
	}

	const std::filesystem::path aeOutputRoot = NANBUSTER_AE_SHADER_OUTPUT_ROOT;
	for (std::size_t index = 0; index < NaNBuster::AELambertShaderIdentities.size(); ++index)
	{
		const auto &identity = NaNBuster::AELambertShaderIdentities[index];
		const auto targetRoot = aeOutputRoot / identity.ReferenceStem;
		ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(NaNBuster::TestFixtures::LoadBinaryFile(
										  (targetRoot / "patched.dxbc").string())));
		const bool overlap =
			index == 1 || index == 2 || index == 3 || index == 5 || index == 7 || index == 9 || index == 11;
		if (overlap)
		{
			for (const auto *filename : {"anisotropic-clamped.dxbc", "combined.dxbc"})
			{
				ExpectPassthrough(allEnabled,
								  NaNBuster::TestFixtures::AsByteView(
									  NaNBuster::TestFixtures::LoadBinaryFile((targetRoot / filename).string())));
			}
		}
	}
	ExpectPassthrough(allEnabled,
					  NaNBuster::TestFixtures::AsByteView(NaNBuster::TestFixtures::LoadBinaryFile(
						  (aeOutputRoot / NaNBuster::AEIsotropicSpecularReferenceStem / "patched.dxbc").string())));

	const auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(corrected));
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{});
	ExpectPassthrough(allEnabled, NaNBuster::ByteView{corrected.data(), 19});

	auto flipped = NaNBuster::TestFixtures::LoadOriginalShader();
	flipped[4] ^= std::byte{0xFF};
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flipped));
	auto flippedIsotropic = NaNBuster::TestFixtures::LoadBinaryFile(NANBUSTER_ISOTROPIC_ORIGINAL_PATH);
	flippedIsotropic[4] ^= std::byte{0xFF};
	ExpectPassthrough(allEnabled, NaNBuster::TestFixtures::AsByteView(flippedIsotropic));

	if (g_failures != 0)
	{
		std::cerr << "Application.ScopedShaderPatchService: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Application.ScopedShaderPatchService: all checks passed (27 identities, 55 entries, 16 policies, "
				 "independent toggles, passthrough).\n";
	return EXIT_SUCCESS;
}
