#pragma warning(push)
#pragma warning(disable : 4267 4244)
#include <F4SE/F4SE.hpp>
#pragma warning(pop)

#include "Infrastructure/ComputeShaderCreationHook.h"

#include <cstddef>
#include <cstdint>

extern "C" alignas(64) void *NaNBuster_ComputeHookState[3] = {nullptr, nullptr, nullptr};

extern "C" void NaNBuster_ComputeDispatchShaderCreation(const std::byte *a_data, std::size_t a_size,
														NaNBuster::HookCreationContext *a_context) noexcept
{
	auto *hook = static_cast<NaNBuster::ComputeShaderCreationHook *>(NaNBuster_ComputeHookState[2]);
	if (hook == nullptr)
	{
		return;
	}
	static_cast<void>(hook->Dispatch(NaNBuster::ByteView{a_data, a_size}, *a_context));
}

namespace NaNBuster
{
ComputeShaderCreationHook::ComputeShaderCreationHook(const ShaderPatchService &service, IEngineShaderCreator &creator,
													 OutcomeLogger logOutcome,
													 SubstitutionLogger logSubstitution) noexcept
	: m_service(service), m_creator(creator), m_logOutcome(logOutcome), m_logSubstitution(logSubstitution)
{
}

auto ComputeShaderCreationHook::Install() noexcept -> bool
{
	if (m_attempted)
	{
		return m_installed;
	}
	m_attempted = true;

	const auto operations = DefaultNativeHookOperations();
	const NativeHookInstallRequest request{
		.Definition = GetComputeShaderHookDefinition(F4SE::GetRuntimeType()),
		.EntryAddress = reinterpret_cast<std::uintptr_t>(&NaNBuster_ComputeHookEntry),
		.HookInstance = this,
		.HookState = NaNBuster_ComputeHookState,
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

auto ComputeShaderCreationHook::IsInstalled() const noexcept -> bool
{
	return m_installed;
}

auto ComputeShaderCreationHook::Dispatch(ByteView originalBytes, const HookCreationContext &context) noexcept
	-> EngineCreationOutcome
{
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

auto ComputeShaderReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext &context) noexcept
	-> std::intptr_t
{
	const auto *hookContext = reinterpret_cast<const HookCreationContext *>(&context);
	return NaNBuster_ComputeReplayOriginalCreation(selectedBytecode.data(), selectedBytecode.size(), *hookContext);
}
} // namespace NaNBuster
