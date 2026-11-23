#include "Infrastructure/Composition.h"

namespace NaNBuster
{
	Composition::Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger) noexcept :
		m_service(m_matcher, m_payload),
		m_creator(replay),
		m_hook(m_service, m_creator, outcomeLogger),
		m_computeCreator(replay),
		m_computeHook(m_service, m_computeCreator, outcomeLogger)
	{
	}

	Composition::Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger, FixPolicy policy) noexcept :
		m_service(m_registry, policy),
		m_creator(replay),
		m_hook(m_service, m_creator, outcomeLogger),
		m_computeCreator(replay),
		m_computeHook(m_service, m_computeCreator, outcomeLogger)
	{
	}

	Composition::Composition(
		OriginalCreationCallback replay,
		OriginalCreationCallback computeReplay,
		OutcomeLogger outcomeLogger,
		FixPolicy policy,
		SubstitutionLogger substitutionLogger) noexcept :
		m_service(m_registry, policy),
		m_creator(replay),
		m_hook(m_service, m_creator, outcomeLogger, substitutionLogger),
		m_computeCreator(computeReplay),
		m_computeHook(m_service, m_computeCreator, outcomeLogger, substitutionLogger)
	{
	}

	auto Composition::Initialize() noexcept -> bool
	{
		const bool pixelInstalled = m_hook.Install();
		const bool computeInstalled = m_computeHook.Install();
		return pixelInstalled && computeInstalled;
	}
}
