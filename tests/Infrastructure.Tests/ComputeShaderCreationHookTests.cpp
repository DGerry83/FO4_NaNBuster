#include "Infrastructure/ComputeShaderCreationHook.h"
#include "Infrastructure/EmbeddedShaderPatchRegistry.h"
#include "TestShaderFixtures.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace
{
	NaNBuster::HookOutcome g_events[8] = {};
	std::size_t g_eventCount = 0;
	std::vector<std::string> g_substitutions;
	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

	void RecordOutcome(NaNBuster::HookOutcome outcome) noexcept
	{
		if (g_eventCount < 8) {
			g_events[g_eventCount++] = outcome;
		}
	}

	void RecordSubstitution(const NaNBuster::ShaderIdentity& identity) noexcept
	{
		g_substitutions.emplace_back(identity.ShaderUid);
	}

	auto EventsEqual(std::initializer_list<NaNBuster::HookOutcome> expected) -> bool
	{
		if (g_eventCount != expected.size()) {
			return false;
		}
		std::size_t index = 0;
		for (const auto event : expected) {
			if (g_events[index++] != event) {
				return false;
			}
		}
		return true;
	}

	struct ScriptedCreator final : NaNBuster::IEngineShaderCreator
	{
		struct Call
		{
			const std::byte* data;
			std::size_t size;
			void* context;
		};

		Call calls[4] = {};
		std::size_t callCount = 0;
		NaNBuster::EngineCreationOutcome scripted[4] = {};
		std::size_t scriptedCount = 0;

		auto Create(NaNBuster::ByteView selectedBytecode, void* creationContext) noexcept
			-> NaNBuster::EngineCreationOutcome override
		{
			if (callCount < 4) {
				calls[callCount] = Call{ selectedBytecode.data(), selectedBytecode.size(), creationContext };
			}
			++callCount;
			if (scriptedCount == 0) {
				return { 0, nullptr };
			}
			return scripted[callCount - 1 < scriptedCount ? callCount - 1 : scriptedCount - 1];
		}
	};

	struct Storage
	{
		unsigned char bytes[16];
	};
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

auto main() -> int
{
	const std::filesystem::path referenceRoot = NANBUSTER_LAMBERT_REFERENCE_ROOT;
	const std::filesystem::path outputRoot = NANBUSTER_LAMBERT_SHADER_OUTPUT_ROOT;
	const auto original = NaNBuster::TestFixtures::LoadBinaryFile(
		(referenceRoot / "e73d9a26-0c9afe98-f628f975-0e5f8aef" / "original.dxbc").string());
	const auto corrected = NaNBuster::TestFixtures::LoadBinaryFile(
		(outputRoot / "e73d9a26-0c9afe98-f628f975-0e5f8aef" / "patched.dxbc").string());
	const auto originalView = NaNBuster::TestFixtures::AsByteView(original);
	const auto correctedView = NaNBuster::TestFixtures::AsByteView(corrected);
	CHECK(originalView.size() == 3892);
	CHECK(correctedView.size() == 4060);

	const NaNBuster::EmbeddedShaderPatchRegistry registry;
	const NaNBuster::ShaderPatchService service(registry, NaNBuster::FixPolicy::AllEnabled());
	Storage deviceStorage, wrapperStorage, bytecodeStorage, slotStorage, siteStackStorage;
	NaNBuster::HookCreationContext context{
		.engine = NaNBuster::OgCreationContext{
			.device = reinterpret_cast<ID3D11Device*>(&deviceStorage),
			.engineWrapper = &wrapperStorage,
			.originalBytecode = &bytecodeStorage,
			.originalSize = 3892,
			.outShaderSlot = &slotStorage,
		},
		.siteStack = &siteStackStorage,
	};
	ScriptedCreator creator;
	NaNBuster::ComputeShaderCreationHook hook(service, creator, &RecordOutcome, &RecordSubstitution);
	CHECK(!hook.IsInstalled());

	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0, &wrapperStorage };
	creator.scriptedCount = 1;
	const auto nonmatch = hook.Dispatch(correctedView, context);
	CHECK(creator.callCount == 1);
	CHECK(std::equal(creator.calls[0].data, creator.calls[0].data + creator.calls[0].size, correctedView.data()));
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(nonmatch.Succeeded());
	CHECK(g_eventCount == 0);
	CHECK(g_substitutions.empty());

	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0, &wrapperStorage };
	creator.scriptedCount = 1;
	g_substitutions.clear();
	const auto patched = hook.Dispatch(originalView, context);
	CHECK(creator.callCount == 1);
	CHECK(std::equal(creator.calls[0].data, creator.calls[0].data + creator.calls[0].size, correctedView.data()));
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(patched.Succeeded());
	CHECK(g_substitutions.size() == 1);
	CHECK(g_substitutions[0] == "e73d9a26-0c9afe98-f628f975-0e5f8aef");
	CHECK(g_eventCount == 0);

	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0x80004005LL, &wrapperStorage };
	creator.scripted[1] = NaNBuster::EngineCreationOutcome{ 0, &slotStorage };
	creator.scriptedCount = 2;
	g_substitutions.clear();
	const auto retry = hook.Dispatch(originalView, context);
	CHECK(creator.callCount == 2);
	CHECK(std::equal(creator.calls[0].data, creator.calls[0].data + creator.calls[0].size, correctedView.data()));
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(creator.calls[1].data == originalView.data());
	CHECK(creator.calls[1].size == originalView.size());
	CHECK(retry.Succeeded());
	CHECK(retry.ShaderHandle == &slotStorage);
	CHECK(g_substitutions.empty());
	CHECK(EventsEqual({ NaNBuster::HookOutcome::CorrectedCreationFailedOriginal }));

	if (g_failures != 0) {
		std::cerr << "Infrastructure.ComputeShaderCreationHook: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}
	std::cout << "Infrastructure.ComputeShaderCreationHook: all checks passed (nonmatch, corrected, retry, identity log).\n";
	return EXIT_SUCCESS;
}
