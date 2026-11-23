#pragma once

#include "Infrastructure/ShaderCreationHook.h"

namespace NaNBuster
{
	class ComputeShaderCreationHook
	{
	public:
		ComputeShaderCreationHook(
			const ShaderPatchService& service,
			IEngineShaderCreator& creator,
			OutcomeLogger logOutcome,
			SubstitutionLogger logSubstitution = nullptr) noexcept;

		auto Install() noexcept -> bool;
		[[nodiscard]] auto IsInstalled() const noexcept -> bool;
		auto Dispatch(ByteView originalBytes, const HookCreationContext& context) noexcept -> EngineCreationOutcome;

	private:
		const ShaderPatchService& m_service;
		IEngineShaderCreator& m_creator;
		OutcomeLogger m_logOutcome;
		SubstitutionLogger m_logSubstitution;
		bool m_attempted = false;
		bool m_installed = false;
	};

	auto ComputeShaderReplayCreationCallback(ByteView selectedBytecode, const OgCreationContext& context) noexcept -> std::intptr_t;
}

extern "C" void NaNBuster_ComputeHookEntry();
extern "C" auto NaNBuster_ComputeReplayOriginalCreation(
	const std::byte* selectedData,
	std::size_t selectedSize,
	const NaNBuster::HookCreationContext& context) noexcept -> std::intptr_t;
