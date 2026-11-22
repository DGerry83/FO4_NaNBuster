#include "Core/ShaderIdentity.h"
#include "Infrastructure/EmbeddedShaderPayload.h"
#include "TestShaderFixtures.h"

#include <algorithm>
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
	// Approved target values, transcribed independently from the plan documents.
	constexpr std::uint32_t ExpectedBytecodeSize = 1612u;
	constexpr std::uint32_t ExpectedCorrectedSize = 1652u;

	static_assert(NaNBuster::OriginalTargetIdentity.BytecodeSize == ExpectedBytecodeSize);

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
	// Fixture-owned oracle storage: the accepted corrected patched.dxbc.
	auto corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	const NaNBuster::ByteView correctedView = NaNBuster::TestFixtures::AsByteView(corrected);
	CHECK(correctedView.size() == ExpectedCorrectedSize);

	const NaNBuster::EmbeddedShaderPayload payload;
	const NaNBuster::ByteView embedded = payload.GetCorrectedBytecode();

	// Size and full byte equality against the accepted patched.dxbc oracle.
	CHECK(embedded.data() != nullptr);
	CHECK(embedded.size() == ExpectedCorrectedSize);
	CHECK(embedded.size() == correctedView.size());
	CHECK(std::equal(embedded.begin(), embedded.end(), correctedView.begin(), correctedView.end()));

	// Exact identity: the reference addresses the OriginalTargetIdentity
	// object itself, and every field matches.
	const NaNBuster::ShaderIdentity& identity = payload.GetSourceIdentity();
	CHECK(&identity == &NaNBuster::OriginalTargetIdentity);
	CHECK(identity.ShaderUid == NaNBuster::OriginalTargetIdentity.ShaderUid);
	CHECK(identity.BytecodeSize == NaNBuster::OriginalTargetIdentity.BytecodeSize);
	CHECK(identity.EmbeddedHash == NaNBuster::OriginalTargetIdentity.EmbeddedHash);
	CHECK(identity.GfxBoosterHash == NaNBuster::OriginalTargetIdentity.GfxBoosterHash);
	CHECK(identity.AssemblyHash == NaNBuster::OriginalTargetIdentity.AssemblyHash);

	// Repeated-view stability: every getter call and every adapter instance
	// exposes the same immutable generated storage.
	const NaNBuster::ByteView again = payload.GetCorrectedBytecode();
	CHECK(again.data() == embedded.data());
	CHECK(again.size() == embedded.size());
	CHECK(std::equal(again.begin(), again.end(), embedded.begin(), embedded.end()));
	const NaNBuster::EmbeddedShaderPayload secondPayload;
	const NaNBuster::ByteView secondView = secondPayload.GetCorrectedBytecode();
	CHECK(secondView.data() == embedded.data());
	CHECK(secondView.size() == embedded.size());
	CHECK(&secondPayload.GetSourceIdentity() == &identity);

	// View validity after adapter destruction: the generated storage is
	// process-lifetime, so a view taken from a destroyed adapter still
	// byte-matches the accepted oracle.
	NaNBuster::ByteView surviving;
	{
		const NaNBuster::EmbeddedShaderPayload scopedPayload;
		surviving = scopedPayload.GetCorrectedBytecode();
	}
	CHECK(surviving.size() == correctedView.size());
	CHECK(std::equal(surviving.begin(), surviving.end(), correctedView.begin(), correctedView.end()));

	if (g_failures != 0) {
		std::cerr << "Infrastructure.EmbeddedShaderPayload: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Infrastructure.EmbeddedShaderPayload: all checks passed (embedded "
			  << embedded.size() << " bytes, corrected oracle " << correctedView.size()
			  << " bytes).\n";
	return EXIT_SUCCESS;
}
