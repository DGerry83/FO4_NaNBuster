// CommonLib first: its headers are not standalone and require the umbrella
// include ahead of any Windows API header (the hook header pulls in d3d11.h).
// The pinned CommonLib's own templates (REX/Crc.hpp via REL::Id, REL/Runtime)
// trip C4267/C4244 under the consumer warning policy; scoped to this TU.
#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/ShaderCreationHook.h"

#include <cstddef>
#include <cstdint>

// Process-lifetime native hook state, read by the MASM entry/replay on every
// intercepted creation. Written once by Install() immediately before the
// detour is published and never mutated afterwards unless installation fails
// before a live detour exists.
extern "C" alignas(64) void *NaNBuster_HookState[3] = {nullptr, nullptr, nullptr};

// Native entry thunk called by the MASM detour entry. Win64 scalar-only ABI:
// rcx = original bytes, rdx = original size, r8 = &HookCreationContext, void
// return. The native entry already resumes the engine with rax = engine
// wrapper, so the dispatch outcome is deliberately discarded and no 16-byte
// aggregate (with a hidden return pointer shifting the argument registers)
// ever enters the native contract.
extern "C" void NaNBuster_DispatchShaderCreation(const std::byte *a_data, std::size_t a_size,
												 NaNBuster::HookCreationContext *a_context) noexcept
{
	auto *hook = static_cast<NaNBuster::ShaderCreationHook *>(NaNBuster_HookState[2]);
	if (hook == nullptr)
	{
		return; // unreachable after a valid install; creates no outcome object
	}
	static_cast<void>(hook->Dispatch(NaNBuster::ByteView{a_data, a_size}, *a_context));
}

namespace NaNBuster
{
ShaderCreationHook::ShaderCreationHook(const ShaderPatchService &service, IEngineShaderCreator &creator,
									   OutcomeLogger logOutcome, SubstitutionLogger logSubstitution) noexcept
	: m_service(service), m_creator(creator), m_logOutcome(logOutcome), m_logSubstitution(logSubstitution)
{
}

auto ShaderCreationHook::Install() noexcept -> bool
{
	if (m_attempted)
	{
		return m_installed;
	}
	m_attempted = true;

	const auto operations = DefaultNativeHookOperations();
	const NativeHookInstallRequest request{
		.Definition = GetPixelShaderHookDefinition(F4SE::GetRuntimeType()),
		.EntryAddress = reinterpret_cast<std::uintptr_t>(&NaNBuster_HookEntry),
		.HookInstance = this,
		.HookState = NaNBuster_HookState,
		.Operations = &operations,
	};
	const auto result = InstallNativeHook(request);
	if (result == NativeHookInstallResult::Unavailable)
	{
		m_logOutcome(HookOutcome::HookUnavailable);
		return false;
	}

	m_installed = true;
	m_logOutcome(HookOutcome::PatchInstalled);
	return true;
}

auto ShaderCreationHook::IsInstalled() const noexcept -> bool
{
	return m_installed;
}

auto ShaderCreationHook::Dispatch(ByteView originalBytes, const HookCreationContext &context) noexcept
	-> EngineCreationOutcome
{
	// No enabled exact match -> caller's original view, Substitute false;
	// enabled exact match -> registry/legacy corrected view.
	const ShaderSelection selection = m_service.Select(originalBytes);
	if (!selection.Substitute)
	{
		return m_creator.Create(selection.SelectedBytecode, const_cast<OgCreationContext *>(&context.engine));
	}

	const EngineCreationOutcome corrected =
		m_creator.Create(selection.SelectedBytecode, const_cast<OgCreationContext *>(&context.engine));
	if (corrected.Succeeded())
	{
		if (m_logSubstitution != nullptr && selection.SourceIdentity != nullptr)
		{
			m_logSubstitution(*selection.SourceIdentity);
		}
		else
		{
			m_logOutcome(HookOutcome::TargetSubstituted);
		}
		return corrected;
	}

	m_logOutcome(HookOutcome::CorrectedCreationFailedOriginal);
	return m_creator.Create(originalBytes, const_cast<OgCreationContext *>(&context.engine));
}

auto HookReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext &context) noexcept -> std::intptr_t
{
	// C11 hands us the address of the OgCreationContext member of the
	// HookCreationContext that Dispatch received; the member sits at offset
	// 0 (static-asserted in the header), so the outer wrapper — and its
	// captured site stack — is recoverable here.
	const auto *hookContext = reinterpret_cast<const HookCreationContext *>(&context);
	return NaNBuster_ReplayOriginalCreation(selectedBytecode.data(), selectedBytecode.size(), *hookContext);
}
} // namespace NaNBuster
