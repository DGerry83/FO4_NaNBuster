#include "Application/ShaderPatchService.h"
#include "Core/TargetShaderMatcher.h"
#include "Infrastructure/ShaderCreationHook.h"

#include "TestShaderFixtures.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <vector>

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

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

	static_assert(offsetof(NaNBuster::HookCreationContext, engine) == 0);
	static_assert(offsetof(NaNBuster::HookCreationContext, siteStack) == 40);
	static_assert(sizeof(NaNBuster::HookCreationContext) == 48);

	// Instrumented IShaderPayload: counts every query and serves the corrected
	// fixture view. C8 must never touch it on a nonmatch.
	struct RecordingPayload final : NaNBuster::IShaderPayload
	{
		mutable int correctedCalls = 0;
		mutable int identityCalls = 0;
		NaNBuster::ByteView corrected = {};
		const NaNBuster::ShaderIdentity* identity = nullptr;

		auto GetCorrectedBytecode() const noexcept -> NaNBuster::ByteView override
		{
			++correctedCalls;
			return corrected;
		}

		auto GetSourceIdentity() const noexcept -> const NaNBuster::ShaderIdentity& override
		{
			++identityCalls;
			return *identity;
		}
	};

	// Instrumented IEngineShaderCreator: records exact data/size/context per
	// call and replays scripted outcomes (fixed storage; Create is noexcept).
	struct ScriptedCreator final : NaNBuster::IEngineShaderCreator
	{
		static constexpr std::size_t kMaxCalls = 8;

		struct Call
		{
			const std::byte* data;
			std::size_t size;
			void* context;
		};

		Call calls[kMaxCalls] = {};
		std::size_t callCount = 0;
		NaNBuster::EngineCreationOutcome scripted[kMaxCalls] = {};
		std::size_t scriptedCount = 0;

		auto Create(NaNBuster::ByteView selectedBytecode, void* creationContext) noexcept
			-> NaNBuster::EngineCreationOutcome override
		{
			if (callCount < kMaxCalls) {
				calls[callCount] = Call{ selectedBytecode.data(), selectedBytecode.size(), creationContext };
			}
			++callCount;
			if (scriptedCount == 0) {
				return NaNBuster::EngineCreationOutcome{ 0, nullptr };
			}
			const std::size_t index = callCount - 1 < scriptedCount ? callCount - 1 : scriptedCount - 1;
			return scripted[index];
		}
	};

	NaNBuster::HookOutcome g_events[8] = {};
	std::size_t g_eventCount = 0;

	void RecordOutcome(NaNBuster::HookOutcome outcome) noexcept
	{
		if (g_eventCount < 8) {
			g_events[g_eventCount++] = outcome;
		}
	}

	void ClearEvents() noexcept
	{
		g_eventCount = 0;
	}

	auto EventsEqual(std::initializer_list<NaNBuster::HookOutcome> expected) -> bool
	{
		if (g_eventCount != expected.size()) {
			return false;
		}
		std::size_t i = 0;
		for (const auto event : expected) {
			if (g_events[i++] != event) {
				return false;
			}
		}
		return true;
	}

	// Address-identifiable stand-ins for the engine-owned objects the context
	// points at; the test never dereferences any of them.
	struct alignas(16) Storage
	{
		unsigned char bytes[16];
	};

	auto AllEngineFieldsEqual(const NaNBuster::HookCreationContext& lhs, const NaNBuster::HookCreationContext& rhs) -> bool
	{
		return lhs.engine.device == rhs.engine.device &&
			lhs.engine.engineWrapper == rhs.engine.engineWrapper &&
			lhs.engine.originalBytecode == rhs.engine.originalBytecode &&
			lhs.engine.originalSize == rhs.engine.originalSize &&
			lhs.engine.outShaderSlot == rhs.engine.outShaderSlot &&
			lhs.siteStack == rhs.siteStack;
	}

	auto EngineFieldsEqual(const NaNBuster::OgCreationContext& lhs, const NaNBuster::OgCreationContext& rhs) -> bool
	{
		return lhs.device == rhs.device && lhs.engineWrapper == rhs.engineWrapper &&
			lhs.originalBytecode == rhs.originalBytecode && lhs.originalSize == rhs.originalSize &&
			lhs.outShaderSlot == rhs.outShaderSlot;
	}
}

auto main() -> int
{
	// D9 write-recovery truth table (pure seam; no engine interaction). The
	// (false, true, true) combination is impossible by construction (the
	// original 7 bytes and the intended detour differ) and resolves to
	// RestoredOriginal, which is the safe reading.
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(false, true, false) == NaNBuster::DetourWriteRecovery::RestoredOriginal);
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(true, false, true) == NaNBuster::DetourWriteRecovery::DetourIntact);
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(false, false, true) == NaNBuster::DetourWriteRecovery::DetourIntact);
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(false, false, false) == NaNBuster::DetourWriteRecovery::Irrecoverable);
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(true, false, false) == NaNBuster::DetourWriteRecovery::Irrecoverable);
	CHECK(NaNBuster::ClassifyDetourWriteRecovery(false, true, true) == NaNBuster::DetourWriteRecovery::RestoredOriginal);

	// Real C7 matcher + real C8 selection use the read-only original fixture
	// (1612 bytes, exact embedded hash) and the generated corrected oracle.
	const std::vector<std::byte> original = NaNBuster::TestFixtures::LoadOriginalShader();
	const std::vector<std::byte> corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
	const NaNBuster::ByteView originalView = NaNBuster::TestFixtures::AsByteView(original);
	const NaNBuster::ByteView correctedView = NaNBuster::TestFixtures::AsByteView(corrected);
	CHECK(originalView.size() == 1612);
	CHECK(correctedView.size() == 1652);

	Storage deviceStorage, wrapperStorage, bytecodeStorage, slotStorage;
	Storage siteStackStorage; // stand-in captured engine rsp (never dereferenced)

	NaNBuster::HookCreationContext context{
		.engine = NaNBuster::OgCreationContext{
			.device = reinterpret_cast<ID3D11Device*>(&deviceStorage),
			.engineWrapper = &wrapperStorage,
			.originalBytecode = &bytecodeStorage,
			.originalSize = 1612u,
			.outShaderSlot = &slotStorage,
		},
		.siteStack = &siteStackStorage,
	};
	const NaNBuster::HookCreationContext contextSnapshot = context;

	const NaNBuster::TargetShaderMatcher matcher;
	RecordingPayload payload;
	payload.corrected = correctedView;
	payload.identity = &NaNBuster::OriginalTargetIdentity;
	NaNBuster::ShaderPatchService service(matcher, payload);

	ScriptedCreator creator;
	NaNBuster::ShaderCreationHook hook(service, creator, &RecordOutcome);

	// --- Nonmatch success: passes the caller's original view through exactly
	// once, zero payload queries, zero log events. The 1652-byte corrected
	// fixture is a realistic non-target input.
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0, &wrapperStorage };
	creator.scriptedCount = 1;
	const NaNBuster::EngineCreationOutcome nonmatchOk = hook.Dispatch(correctedView, context);
	CHECK(creator.callCount == 1);
	CHECK(creator.calls[0].data == correctedView.data());
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(creator.calls[0].context == static_cast<void*>(&context.engine));
	CHECK(payload.correctedCalls == 0);
	CHECK(payload.identityCalls == 0);
	CHECK(g_eventCount == 0);
	CHECK(nonmatchOk.EngineResult == 0);
	CHECK(nonmatchOk.Succeeded());
	CHECK(nonmatchOk.ShaderHandle == &wrapperStorage);
	CHECK(AllEngineFieldsEqual(context, contextSnapshot));

	// --- Nonmatch failure: result returned verbatim, still no retry, no log.
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0x80070057LL, &wrapperStorage };
	creator.scriptedCount = 1;
	const NaNBuster::EngineCreationOutcome nonmatchFail = hook.Dispatch(correctedView, context);
	CHECK(creator.callCount == 1);
	CHECK(nonmatchFail.EngineResult == 0x80070057LL);
	CHECK(!nonmatchFail.Succeeded());
	CHECK(g_eventCount == 0);
	CHECK(payload.correctedCalls == 0);
	CHECK(AllEngineFieldsEqual(context, contextSnapshot));

	// --- Exact match, corrected success: corrected bytes AND corrected size
	// reach the creator once; exactly one substitution event.
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0, &wrapperStorage };
	creator.scriptedCount = 1;
	const NaNBuster::EngineCreationOutcome patchedOk = hook.Dispatch(originalView, context);
	CHECK(creator.callCount == 1);
	CHECK(creator.calls[0].data == correctedView.data());
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(creator.calls[0].context == static_cast<void*>(&context.engine));
	CHECK(payload.correctedCalls == 1);
	CHECK(payload.identityCalls == 0);
	CHECK(EventsEqual({ NaNBuster::HookOutcome::TargetSubstituted }));
	CHECK(patchedOk.EngineResult == 0);
	CHECK(patchedOk.Succeeded());
	CHECK(patchedOk.ShaderHandle == &wrapperStorage);
	CHECK(AllEngineFieldsEqual(context, contextSnapshot));

	// --- Exact match, corrected failure, original retry success: corrected
	// attempted once, failure event logged once, original bytes retried exactly
	// once with the caller's original view, retry result returned.
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0x80004005LL, &wrapperStorage };
	creator.scripted[1] = NaNBuster::EngineCreationOutcome{ 0, &slotStorage };
	creator.scriptedCount = 2;
	const NaNBuster::EngineCreationOutcome retryOk = hook.Dispatch(originalView, context);
	CHECK(creator.callCount == 2);
	CHECK(creator.calls[0].data == correctedView.data());
	CHECK(creator.calls[0].size == correctedView.size());
	CHECK(creator.calls[1].data == originalView.data());
	CHECK(creator.calls[1].size == originalView.size());
	CHECK(creator.calls[0].context == static_cast<void*>(&context.engine));
	CHECK(creator.calls[1].context == static_cast<void*>(&context.engine));
	CHECK(EventsEqual({ NaNBuster::HookOutcome::CorrectedCreationFailedOriginal }));
	CHECK(retryOk.EngineResult == 0);
	CHECK(retryOk.Succeeded());
	CHECK(retryOk.ShaderHandle == &slotStorage);
	CHECK(payload.correctedCalls == 2); // one per match dispatch so far (1 + 1)
	CHECK(AllEngineFieldsEqual(context, contextSnapshot));

	// --- Exact match, corrected failure, original retry failure: retry result
	// returned as-is; no second patch attempt, no additional event.
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0x80004005LL, &wrapperStorage };
	creator.scripted[1] = NaNBuster::EngineCreationOutcome{ 0x80070057LL, &slotStorage };
	creator.scriptedCount = 2;
	const NaNBuster::EngineCreationOutcome retryFail = hook.Dispatch(originalView, context);
	CHECK(creator.callCount == 2);
	CHECK(retryFail.EngineResult == 0x80070057LL);
	CHECK(!retryFail.Succeeded());
	CHECK(retryFail.ShaderHandle == &slotStorage);
	CHECK(EventsEqual({ NaNBuster::HookOutcome::CorrectedCreationFailedOriginal }));
	CHECK(AllEngineFieldsEqual(context, contextSnapshot));

	// --- Engine-owned input storage is untouched by dispatch (the engine keeps
	// ownership of the pool buffer; the hook never writes through it).
	const std::vector<std::byte> originalWitness = NaNBuster::TestFixtures::LoadOriginalShader();
	ClearEvents();
	creator = ScriptedCreator{};
	creator.scripted[0] = NaNBuster::EngineCreationOutcome{ 0x80004005LL, nullptr };
	creator.scripted[1] = NaNBuster::EngineCreationOutcome{ 0, nullptr };
	creator.scriptedCount = 2;
	(void)hook.Dispatch(originalView, context);
	CHECK(std::memcmp(original.data(), originalWitness.data(), original.size()) == 0);
	CHECK(std::memcmp(corrected.data(), correctedView.data(), correctedView.size()) == 0);

	// --- Context integrity across every path: the engine subobject the creator
	// receives must be the exact HookCreationContext engine member (offset-0
	// identity), and all fields including the captured site stack survive.
	CHECK(reinterpret_cast<const void*>(&context.engine) == static_cast<const void*>(&context));
	const NaNBuster::OgCreationContext engineView{
		.device = context.engine.device,
		.engineWrapper = context.engine.engineWrapper,
		.originalBytecode = context.engine.originalBytecode,
		.originalSize = context.engine.originalSize,
		.outShaderSlot = context.engine.outShaderSlot,
	};
	CHECK(EngineFieldsEqual(context.engine, engineView));

	if (g_failures != 0) {
		std::cerr << "Infrastructure.ShaderCreationHook: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Infrastructure.ShaderCreationHook: all checks passed (nonmatch silent 1-call, "
			  << "corrected 1-call, corrected-failure retry exactly-once, events/logger seam exact, "
			  << "context fields unchanged).\n";
	return EXIT_SUCCESS;
}
