#pragma once

#include "Application/IEngineShaderCreator.h"
#include "Application/ShaderPatchService.h"
#include "Infrastructure/EngineShaderCreator.h"
#include "Infrastructure/NativeHookInstaller.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace NaNBuster
{
// C12-native wrapper around the unchanged C11 OgCreationContext
// (G5_HOOK_CONTRACT.md §6.3 and AE_HOOK_CONTRACT.md §6): the native hook
// entry captures the engine stack pointer at the active OG/AE site so the
// replay can honor the verified frame slots. engine is the FIRST member at offset 0, so a
// HookCreationContext* addresses its engine subobject — that is what lets
// the C11-compatible callback recover the wrapper (static-asserted here and
// mirrored by .errnz checks in ShaderCreationHook.asm).
struct HookCreationContext
{
	OgCreationContext engine; // offset 0
	void *siteStack;		  // captured engine rsp at the hook site
};

static_assert(std::is_standard_layout_v<HookCreationContext>);
static_assert(offsetof(HookCreationContext, engine) == 0);
static_assert(offsetof(HookCreationContext, siteStack) == 40);
static_assert(sizeof(HookCreationContext) == 48);
static_assert(offsetof(OgCreationContext, device) == 0);
static_assert(offsetof(OgCreationContext, engineWrapper) == 8);
static_assert(offsetof(OgCreationContext, originalBytecode) == 16);
static_assert(offsetof(OgCreationContext, originalSize) == 24);
static_assert(offsetof(OgCreationContext, outShaderSlot) == 32);
static_assert(sizeof(OgCreationContext) == 40);

// The four approved outcome events. They map 1:1 onto the four locked
// PluginLogger strings; PluginLoggerOutcomeLogger() is the production seam
// adapter. NotTarget is intentionally absent: nonmatches stay silent.
enum class HookOutcome
{
	PatchInstalled,
	TargetSubstituted,
	HookUnavailable,
	CorrectedCreationFailedOriginal,
};

using OutcomeLogger = void (*)(HookOutcome outcome) noexcept;
using SubstitutionLogger = void (*)(const ShaderIdentity &identity) noexcept;

// OG/AE shader-creation hook. Install() selects the active runtime's exact
// definition, verifies its complete fingerprint, then publishes the 7-byte
// JMP+2-NOP detour exactly once. Dispatch() is the testable C++ decision core
// over C8 selection and C11 creation; the MASM native entry
// (NaNBuster_HookEntry) builds the context and calls it.
// Acquires no engine object/handle ownership; the only state is the
// intentional process-lifetime detour plus branch memory (C12 contract).
class ShaderCreationHook
{
  public:
	ShaderCreationHook(const ShaderPatchService &service, IEngineShaderCreator &creator, OutcomeLogger logOutcome,
					   SubstitutionLogger logSubstitution = nullptr) noexcept;

	auto Install() noexcept -> bool;
	[[nodiscard]] auto IsInstalled() const noexcept -> bool;

	auto Dispatch(ByteView originalBytes, const HookCreationContext &context) noexcept -> EngineCreationOutcome;

  private:
	const ShaderPatchService &m_service;
	IEngineShaderCreator &m_creator;
	OutcomeLogger m_logOutcome;
	SubstitutionLogger m_logSubstitution;
	bool m_attempted = false;
	bool m_installed = false;
};

// C12 trampoline-specific body for C11's OriginalCreationCallback
// (EngineShaderCreator constructor injection point). Recovers the
// HookCreationContext from the OgCreationContext reference (offset-0 member,
// static-asserted above) and forwards to the MASM replay routine. Performs
// exactly one original device call per invocation and returns the post-call
// HRESULT/eax. Tests never invoke this callback: it reaches native code.
auto HookReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext &context) noexcept -> std::intptr_t;

// Production OutcomeLogger adapter mapping the four HookOutcome events to
// the four locked PluginLogger strings. Defined in PluginLogger.cpp (the
// runtime DLL translation unit); C13 composition injects it.
auto PluginLoggerOutcomeLogger() noexcept -> OutcomeLogger;
auto PluginLoggerSubstitutionLogger() noexcept -> SubstitutionLogger;
} // namespace NaNBuster

// MASM native routines (ShaderCreationHook.asm), linked into the hook
// component. NaNBuster_HookEntry is the engine detour target;
// NaNBuster_ReplayOriginalCreation is the original-call replay body used by
// HookReplayCreationCallback.
extern "C" void NaNBuster_HookEntry();
extern "C" auto NaNBuster_ReplayOriginalCreation(const std::byte *selectedData, std::size_t selectedSize,
												 const NaNBuster::HookCreationContext &context) noexcept
	-> std::intptr_t;
