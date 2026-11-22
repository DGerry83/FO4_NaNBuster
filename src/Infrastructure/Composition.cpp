#include "Infrastructure/Composition.h"

namespace NaNBuster
{
	Composition::Composition(OriginalCreationCallback replay, OutcomeLogger outcomeLogger) noexcept :
		m_service(m_matcher, m_payload),
		m_creator(replay),
		m_hook(m_service, m_creator, outcomeLogger)
	{
	}

	auto Composition::Initialize() noexcept -> bool
	{
		return m_hook.Install();
	}
}
