#include "Application/IEngineShaderCreator.h"
#include "Infrastructure/EngineShaderCreator.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

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

	// Instrumented original-creation callback: records exactly what the adapter
	// forwarded and returns the injected native result. C11 tests use this in
	// place of the C12 trampoline-specific callback and claim no runtime hook
	// proof.
	struct Probe
	{
		int calls = 0;
		NaNBuster::ByteView receivedBytes = {};
		const NaNBuster::OgCreationContext* receivedContext = nullptr;
		std::intptr_t resultToReturn = 0;
	};

	Probe g_probe;

	auto InstrumentedCreate(NaNBuster::ByteView selectedBytecode, const NaNBuster::OgCreationContext& context) noexcept
		-> std::intptr_t
	{
		++g_probe.calls;
		g_probe.receivedBytes = selectedBytecode;
		g_probe.receivedContext = &context;
		return g_probe.resultToReturn;
	}

	Probe g_secondProbe;

	auto SecondInstrumentedCreate(NaNBuster::ByteView, const NaNBuster::OgCreationContext&) noexcept
		-> std::intptr_t
	{
		++g_secondProbe.calls;
		return 0x1234;
	}

	// Address-identifiable stand-ins for the engine-owned objects the context
	// points at; the test never dereferences any of them.
	struct alignas(16) Storage
	{
		unsigned char bytes[16];
	};

	auto AllFieldsEqual(const NaNBuster::OgCreationContext& lhs, const NaNBuster::OgCreationContext& rhs) -> bool
	{
		return lhs.device == rhs.device && lhs.engineWrapper == rhs.engineWrapper &&
			lhs.originalBytecode == rhs.originalBytecode && lhs.originalSize == rhs.originalSize &&
			lhs.outShaderSlot == rhs.outShaderSlot;
	}
}

auto main() -> int
{
	Storage deviceStorage, wrapperStorage, bytecodeStorage, slotStorage;

	NaNBuster::OgCreationContext context{
		.device = reinterpret_cast<ID3D11Device*>(&deviceStorage),
		.engineWrapper = &wrapperStorage,
		.originalBytecode = &bytecodeStorage,
		.originalSize = 1612u,
		.outShaderSlot = &slotStorage,
	};
	const NaNBuster::OgCreationContext snapshot = context;

	alignas(16) std::byte selected[] = {
		std::byte{ 0x01 }, std::byte{ 0x23 }, std::byte{ 0x45 }, std::byte{ 0x67 },
		std::byte{ 0x89 }, std::byte{ 0xAB }, std::byte{ 0xCD }, std::byte{ 0xEF },
	};
	const NaNBuster::ByteView selectedView{ selected };

	// Create is non-const per the locked boundary; the adapter holds the
	// injected callback and nothing else.
	NaNBuster::EngineShaderCreator creator(InstrumentedCreate);

	// S_OK path: selected data pointer/size and the exact context pointer reach
	// the callback unchanged; every context field survives; the native zero
	// result maps to Succeeded() and the context wrapper becomes ShaderHandle.
	g_probe = Probe{ .resultToReturn = 0 };
	const NaNBuster::EngineCreationOutcome success = creator.Create(selectedView, &context);
	CHECK(g_probe.calls == 1);
	CHECK(g_probe.receivedBytes.data() == selectedView.data());
	CHECK(g_probe.receivedBytes.size() == selectedView.size());
	CHECK(g_probe.receivedContext == &context);
	CHECK(AllFieldsEqual(context, snapshot));
	CHECK(success.EngineResult == 0);
	CHECK(success.Succeeded());
	CHECK(success.ShaderHandle == context.engineWrapper);

	// Injected failure path: exactly one callback call (no hidden retry), the
	// failure value maps verbatim, Succeeded() is false, and ShaderHandle still
	// comes from the context wrapper.
	g_probe = Probe{ .resultToReturn = 0x80004005LL };
	const NaNBuster::EngineCreationOutcome failed = creator.Create(selectedView, &context);
	CHECK(g_probe.calls == 1);
	CHECK(failed.EngineResult == 0x80004005LL);
	CHECK(!failed.Succeeded());
	CHECK(failed.ShaderHandle == context.engineWrapper);
	CHECK(AllFieldsEqual(context, snapshot));

	// Result mapping matrix: Succeeded() is true only for zero; every other
	// native value, including a negative HRESULT-style value, maps exactly.
	constexpr std::intptr_t kResults[] = { 0, 1, -1, 0x80070057LL };
	constexpr bool kExpectedSucceeded[] = { true, false, false, false };
	for (std::size_t i = 0; i < std::size(kResults); ++i) {
		g_probe = Probe{ .resultToReturn = kResults[i] };
		const NaNBuster::EngineCreationOutcome outcome = creator.Create(selectedView, &context);
		CHECK(g_probe.calls == 1);
		CHECK(outcome.EngineResult == kResults[i]);
		CHECK(outcome.Succeeded() == kExpectedSucceeded[i]);
		CHECK(outcome.ShaderHandle == context.engineWrapper);
	}

	// Repeated independent Create calls: each call forwards its own bytes and
	// context and invokes the callback exactly once, with per-call mapping —
	// C11 behavior is per Create, not C12 fallback orchestration.
	Storage device2, wrapper2, bytecode2, slot2;
	NaNBuster::OgCreationContext secondContext{
		.device = reinterpret_cast<ID3D11Device*>(&device2),
		.engineWrapper = &wrapper2,
		.originalBytecode = &bytecode2,
		.originalSize = 1652u,
		.outShaderSlot = &slot2,
	};
	const NaNBuster::OgCreationContext secondSnapshot = secondContext;

	alignas(16) std::byte otherSelected[] = {
		std::byte{ 0xFE }, std::byte{ 0xDC }, std::byte{ 0xBA }, std::byte{ 0x98 },
	};
	const NaNBuster::ByteView otherView{ otherSelected };

	g_probe = Probe{ .resultToReturn = 0 };
	const NaNBuster::EngineCreationOutcome firstOfPair = creator.Create(selectedView, &context);
	CHECK(g_probe.calls == 1);
	CHECK(firstOfPair.Succeeded());
	CHECK(firstOfPair.ShaderHandle == context.engineWrapper);
	CHECK(g_probe.receivedBytes.data() == selectedView.data());
	CHECK(g_probe.receivedBytes.size() == selectedView.size());
	CHECK(g_probe.receivedContext == &context);

	g_probe.resultToReturn = 0x80070057LL;
	const NaNBuster::EngineCreationOutcome secondOfPair = creator.Create(otherView, &secondContext);
	CHECK(g_probe.calls == 2);
	CHECK(secondOfPair.EngineResult == 0x80070057LL);
	CHECK(!secondOfPair.Succeeded());
	CHECK(secondOfPair.ShaderHandle == secondContext.engineWrapper);
	CHECK(g_probe.receivedBytes.data() == otherView.data());
	CHECK(g_probe.receivedBytes.size() == otherView.size());
	CHECK(g_probe.receivedContext == &secondContext);
	CHECK(AllFieldsEqual(context, snapshot));
	CHECK(AllFieldsEqual(secondContext, secondSnapshot));

	// Injection identity: a second adapter holding the other callback invokes
	// only that callback and maps its result.
	NaNBuster::EngineShaderCreator secondCreator(SecondInstrumentedCreate);
	const NaNBuster::EngineCreationOutcome third = secondCreator.Create(selectedView, &context);
	CHECK(g_secondProbe.calls == 1);
	CHECK(third.EngineResult == 0x1234);
	CHECK(!third.Succeeded());
	CHECK(g_probe.calls == 2);

	if (g_failures != 0) {
		std::cerr << "Infrastructure.EngineShaderCreator: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Infrastructure.EngineShaderCreator: all checks passed (per-create callback calls 1, "
			  << "context fields preserved, results mapped verbatim).\n";
	return EXIT_SUCCESS;
}
