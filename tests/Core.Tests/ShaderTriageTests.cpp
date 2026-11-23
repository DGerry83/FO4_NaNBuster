#include "Core/ShaderTriage.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace
{
	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

	auto ReadText(const char* path) -> std::string
	{
		std::ifstream stream(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

int main()
{
	using enum NaNBuster::ShaderPowerFamily;
	using enum NaNBuster::ShaderTriageVerdict;

	const auto original = NaNBuster::AnalyzeShaderDisassembly(ReadText(NANBUSTER_ORIGINAL_ASM_PATH));
	CHECK(original.Verdict == UnguardedReachesOutput);
	CHECK(original.PowerFamily == Rgba22);
	CHECK(original.ColorRegister == "v2");
	CHECK(!original.FlowSignature.empty());

	const auto corrected = NaNBuster::AnalyzeShaderDisassembly(ReadText(NANBUSTER_CORRECTED_ASM_PATH));
	CHECK(corrected.Verdict == GuardedBeforeLog);
	CHECK(corrected.PowerFamily == Rgba22);
	CHECK(corrected.ColorRegister == "v2");

	const auto laterMax = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: mov o0.xyzw, r0.xyzw
4: max r1.xyzw, v2.xyzw, l(0, 0, 0, 0)
)");
	CHECK(laterMax.Verdict == UnguardedReachesOutput);

	const auto dead = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: mov r0.xyzw, l(1, 1, 1, 1)
4: mov o0.xyzw, r0.xyzw
)");
	CHECK(dead.Verdict == UnguardedNoObservedOutput);
	CHECK(!dead.RequiresManualReview);

	const auto channelCleared = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: mov r1.xyzw, l(0, 0, 0, 0)
4: mov r1.x, r0.x
5: mov o0.y, r1.y
)");
	CHECK(channelCleared.Verdict == UnguardedNoObservedOutput);

	const auto alpha = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.x, v2.w
1: mul r0.x, r0.x, l(2.200000)
2: exp r0.x, r0.x
3: mov o0.w, r0.x
)");
	CHECK(alpha.Verdict == UnguardedReachesOutput);
	CHECK(alpha.PowerFamily == Alpha22);

	const auto conversion = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: ftoi r1.x, r0.x
4: mov o0.x, r1.x
)");
	CHECK(conversion.Verdict == Uncertain);
	CHECK(conversion.RequiresManualReview);

	const auto controlFlow = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: if_nz r0.x
4: mov o0.xyzw, r1.xyzw
5: endif
)");
	CHECK(controlFlow.Verdict == Uncertain);
	CHECK(controlFlow.RequiresManualReview);

	const auto unsupported = NaNBuster::AnalyzeShaderDisassembly(R"(
0: log r0.xyzw, v2.xyzw
1: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
2: exp r0.xyzw, r0.xyzw
3: unknown r1.xyzw, r0.xyzw
4: mov o0.xyzw, r1.xyzw
)");
	CHECK(unsupported.Verdict == UnguardedReachesOutput);
	CHECK(unsupported.RequiresManualReview);

	const auto partialGuard = NaNBuster::AnalyzeShaderDisassembly(R"(
0: max r0.xyz, v2.xyz, l(0, 0, 0)
1: log r0.xyzw, r0.xyzw
2: mul r0.xyzw, r0.xyzw, l(2.200000, 2.200000, 2.200000, 2.200000)
3: exp r0.xyzw, r0.xyzw
4: mov o0.xyzw, r0.xyzw
)");
	CHECK(partialGuard.Verdict == Uncertain);
	CHECK(partialGuard.RequiresManualReview);

	const auto malformed = NaNBuster::AnalyzeShaderDisassembly("not a shader\n");
	CHECK(malformed.Verdict == NotCandidate);

	if (g_failures != 0) {
		std::cerr << g_failures << " ShaderTriage checks failed\n";
		return 1;
	}
	std::cout << "All ShaderTriage checks passed\n";
	return 0;
}
