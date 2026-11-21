#include "Application/IShaderPayload.h"
#include "Application/ShaderPatchService.h"
#include "Application/ShaderSelection.h"
#include "Core/ShaderIdentity.h"
#include "Core/TargetShaderMatcher.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#ifndef NANBUSTER_CORRECTED_SHADER_PATH
#	error "NANBUSTER_CORRECTED_SHADER_PATH must be defined by the test target (absolute path to the generated corrected DXBC)."
#endif

namespace
{
	// Test-side knowledge of the approved target values, transcribed
	// independently from the plan documents.
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;
	constexpr std::size_t DxbcEmbeddedHashSize = 16;
	constexpr std::uint32_t ExpectedBytecodeSize = 1612u;
	constexpr std::uint32_t ExpectedCorrectedSize = 1652u;
	constexpr std::array<std::byte, DxbcEmbeddedHashSize> ExpectedEmbeddedHash = {
		std::byte{ 0x59 }, std::byte{ 0xe1 }, std::byte{ 0xf7 }, std::byte{ 0x82 },
		std::byte{ 0xa4 }, std::byte{ 0xb7 }, std::byte{ 0xb6 }, std::byte{ 0x8d },
		std::byte{ 0xf3 }, std::byte{ 0xff }, std::byte{ 0x42 }, std::byte{ 0xb7 },
		std::byte{ 0xe9 }, std::byte{ 0x24 }, std::byte{ 0x13 }, std::byte{ 0xb1 },
	};

	static_assert(NaNBuster::OriginalTargetIdentity.BytecodeSize == ExpectedBytecodeSize);
	static_assert(NaNBuster::OriginalTargetIdentity.EmbeddedHash == ExpectedEmbeddedHash);

	// Fake payload: returns a corrected ByteView over storage distinct from the
	// original fixture and counts every getter call. The identity is
	// OriginalTargetIdentity by value, as the real C9 payload will provide.
	class CountingPayload final : public NaNBuster::IShaderPayload
	{
	public:
		explicit CountingPayload(NaNBuster::ByteView corrected)
			: m_corrected(corrected)
		{
		}

		auto GetCorrectedBytecode() const noexcept -> NaNBuster::ByteView override
		{
			++m_correctedCalls;
			return m_corrected;
		}

		auto GetSourceIdentity() const noexcept -> const NaNBuster::ShaderIdentity& override
		{
			++m_identityCalls;
			return NaNBuster::OriginalTargetIdentity;
		}

		int CorrectedCalls() const noexcept { return m_correctedCalls; }
		int IdentityCalls() const noexcept { return m_identityCalls; }

	private:
		NaNBuster::ByteView m_corrected;
		mutable int m_correctedCalls = 0;
		mutable int m_identityCalls = 0;
	};

	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

auto main() -> int
{
	const NaNBuster::TargetShaderMatcher matcher;

	// Fixture-owned storage; every view below stays within its lifetime.
	auto original = NaNBuster::TestFixtures::LoadOriginalShader();
	const NaNBuster::ByteView originalView = NaNBuster::TestFixtures::AsByteView(original);
	CHECK(originalView.size() == ExpectedBytecodeSize);

	auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	const NaNBuster::ByteView correctedView = NaNBuster::TestFixtures::AsByteView(corrected);
	CHECK(correctedView.size() == ExpectedCorrectedSize);

	CountingPayload payload(correctedView);
	CHECK(payload.CorrectedCalls() == 0);
	CHECK(payload.IdentityCalls() == 0);

	const NaNBuster::ShaderPatchService service(matcher, payload);

	// The fake payload's corrected storage is distinct from the original.
	CHECK(correctedView.data() != originalView.data());
	CHECK(correctedView.size() == ExpectedCorrectedSize);

	// Direct payload access: identity equals OriginalTargetIdentity by value;
	// corrected view is stable across calls.
	{
		const NaNBuster::ShaderIdentity& identity = payload.GetSourceIdentity();
		CHECK(identity.ShaderUid == NaNBuster::OriginalTargetIdentity.ShaderUid);
		CHECK(identity.BytecodeSize == NaNBuster::OriginalTargetIdentity.BytecodeSize);
		CHECK(identity.EmbeddedHash == NaNBuster::OriginalTargetIdentity.EmbeddedHash);
		CHECK(identity.GfxBoosterHash == NaNBuster::OriginalTargetIdentity.GfxBoosterHash);
		CHECK(identity.AssemblyHash == NaNBuster::OriginalTargetIdentity.AssemblyHash);
		CHECK(payload.GetCorrectedBytecode().data() == correctedView.data());
		CHECK(payload.GetCorrectedBytecode().size() == correctedView.size());
	}
	const int baselineCorrectedCalls = payload.CorrectedCalls();
	const int baselineIdentityCalls = payload.IdentityCalls();
	CHECK(baselineCorrectedCalls == 2);
	CHECK(baselineIdentityCalls == 1);

	// Exact original selects the corrected bytes: Patched/true with the
	// payload's corrected view, querying only GetCorrectedBytecode.
	{
		const NaNBuster::ShaderSelection selection = service.Select(originalView);
		CHECK(selection.Outcome == NaNBuster::PatchOutcome::Patched);
		CHECK(selection.Substitute == true);
		CHECK(selection.SelectedBytecode.data() == correctedView.data());
		CHECK(selection.SelectedBytecode.size() == correctedView.size());
		CHECK(payload.CorrectedCalls() == baselineCorrectedCalls + 1);
		CHECK(payload.IdentityCalls() == baselineIdentityCalls);
	}

	// Passthrough helper: view returned identical to input, NotTarget/false,
	// and zero additional payload calls of either kind.
	auto ExpectPassthrough = [&service, &payload, baselineCorrectedCalls, baselineIdentityCalls](NaNBuster::ByteView input, const char* tag) {
		const NaNBuster::ShaderSelection selection = service.Select(input);
		Check(selection.Outcome == NaNBuster::PatchOutcome::NotTarget, tag, __FILE__, __LINE__);
		Check(selection.Substitute == false, tag, __FILE__, __LINE__);
		Check(selection.SelectedBytecode.data() == input.data(), tag, __FILE__, __LINE__);
		Check(selection.SelectedBytecode.size() == input.size(), tag, __FILE__, __LINE__);
		Check(payload.CorrectedCalls() == baselineCorrectedCalls + 1, tag, __FILE__, __LINE__);
		Check(payload.IdentityCalls() == baselineIdentityCalls, tag, __FILE__, __LINE__);
	};

	// Corrected oracle itself (1652 bytes, different embedded hash) is not the
	// original target and passes through unchanged.
	ExpectPassthrough(correctedView, "corrected oracle passthrough");

	// Empty and truncated views pass through.
	ExpectPassthrough(NaNBuster::ByteView{}, "empty passthrough");
	ExpectPassthrough(NaNBuster::ByteView{ original.data(), 4 }, "truncated-4 passthrough");
	ExpectPassthrough(NaNBuster::ByteView{ original.data(), 19 }, "truncated-19 passthrough");

	// Wrong sizes around the exact target size pass through.
	ExpectPassthrough(NaNBuster::ByteView{ original.data(), ExpectedBytecodeSize - 1 }, "size-1611 passthrough");
	std::vector<std::byte> oversized(original);
	oversized.push_back(std::byte{ 0x00 });
	CHECK(oversized.size() == ExpectedBytecodeSize + 1);
	ExpectPassthrough(NaNBuster::TestFixtures::AsByteView(oversized), "size-1613 passthrough");

	// Same-size wrong hash passes through: zeros, first/last embedded-hash
	// byte flipped, and an arbitrary third-party buffer.
	const std::vector<std::byte> zeroed(ExpectedBytecodeSize, std::byte{ 0x00 });
	ExpectPassthrough(NaNBuster::TestFixtures::AsByteView(zeroed), "zeroed-hash passthrough");

	std::vector<std::byte> flipFirst(original);
	flipFirst[DxbcEmbeddedHashOffset] ^= std::byte{ 0xFF };
	ExpectPassthrough(NaNBuster::TestFixtures::AsByteView(flipFirst), "flip-first-hash passthrough");

	std::vector<std::byte> flipLast(original);
	flipLast[DxbcEmbeddedHashOffset + DxbcEmbeddedHashSize - 1] ^= std::byte{ 0xFF };
	ExpectPassthrough(NaNBuster::TestFixtures::AsByteView(flipLast), "flip-last-hash passthrough");

	const std::vector<std::byte> thirdParty(ExpectedBytecodeSize, std::byte{ 0xAA });
	CHECK(!std::equal(
		NaNBuster::OriginalTargetIdentity.EmbeddedHash.begin(),
		NaNBuster::OriginalTargetIdentity.EmbeddedHash.end(),
		thirdParty.data() + DxbcEmbeddedHashOffset));
	ExpectPassthrough(NaNBuster::TestFixtures::AsByteView(thirdParty), "third-party passthrough");

	// Repeated calls are deterministic and re-query the payload only per match.
	{
		const NaNBuster::ShaderSelection first = service.Select(originalView);
		const NaNBuster::ShaderSelection second = service.Select(originalView);
		CHECK(first.Outcome == NaNBuster::PatchOutcome::Patched);
		CHECK(second.Outcome == NaNBuster::PatchOutcome::Patched);
		CHECK(first.Substitute == true);
		CHECK(second.Substitute == true);
		CHECK(first.SelectedBytecode.data() == correctedView.data());
		CHECK(second.SelectedBytecode.data() == correctedView.data());
		CHECK(payload.CorrectedCalls() == baselineCorrectedCalls + 3);

		const NaNBuster::ShaderSelection passA = service.Select(NaNBuster::TestFixtures::AsByteView(flipFirst));
		const NaNBuster::ShaderSelection passB = service.Select(NaNBuster::TestFixtures::AsByteView(flipFirst));
		CHECK(passA.Outcome == NaNBuster::PatchOutcome::NotTarget);
		CHECK(passB.Outcome == NaNBuster::PatchOutcome::NotTarget);
		CHECK(passA.SelectedBytecode.data() == flipFirst.data());
		CHECK(passB.SelectedBytecode.data() == flipFirst.data());
		CHECK(payload.CorrectedCalls() == baselineCorrectedCalls + 3);
		CHECK(payload.IdentityCalls() == baselineIdentityCalls);
	}

	if (g_failures != 0) {
		std::cerr << "Application.ShaderPatchService: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Application.ShaderPatchService: all checks passed (original "
			  << originalView.size() << " bytes, corrected " << correctedView.size()
			  << " bytes, payload calls corrected=" << payload.CorrectedCalls()
			  << " identity=" << payload.IdentityCalls() << ").\n";
	return EXIT_SUCCESS;
}
