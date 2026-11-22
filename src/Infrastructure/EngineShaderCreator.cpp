#include "Infrastructure/EngineShaderCreator.h"

namespace NaNBuster
{
	EngineShaderCreator::EngineShaderCreator(OriginalCreationCallback callback) noexcept :
		m_callback(callback)
	{
	}

	auto EngineShaderCreator::Create(ByteView selectedBytecode, void* creationContext) noexcept
		-> EngineCreationOutcome
	{
		const auto* context = static_cast<const OgCreationContext*>(creationContext);
		const std::intptr_t result = m_callback(selectedBytecode, *context);
		return EngineCreationOutcome{ .EngineResult = result, .ShaderHandle = context->engineWrapper };
	}
}
