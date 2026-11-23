#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace NaNBuster
{
	enum class ShaderTriageVerdict
	{
		NotCandidate,
		GuardedBeforeLog,
		UnguardedReachesOutput,
		UnguardedNoObservedOutput,
		Uncertain,
	};

	enum class ShaderPowerFamily
	{
		NotApplicable,
		Rgba22,
		Rgb22,
		Alpha22,
		Mixed22,
		Other,
		Unknown,
	};

	struct ShaderTriageResult
	{
		ShaderTriageVerdict Verdict = ShaderTriageVerdict::NotCandidate;
		ShaderPowerFamily PowerFamily = ShaderPowerFamily::NotApplicable;
		bool RequiresManualReview = false;
		std::string ColorRegister;
		std::string FlowSignature;
		std::vector<std::string> Evidence;
	};

	auto AnalyzeShaderDisassembly(std::string_view disassembly) -> ShaderTriageResult;
	auto ToString(ShaderTriageVerdict verdict) -> std::string_view;
	auto ToString(ShaderPowerFamily family) -> std::string_view;
}
