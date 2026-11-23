#pragma once

#include "Core/DxbcContainer.h"
#include "Core/LambertShaderIdentities.h"

#include <cstddef>
#include <expected>
#include <vector>

namespace NaNBuster
{
	struct DxbcLambertGuardPatchResult
	{
		std::vector<std::byte> Bytes;
		std::size_t InsertionTokenOffset;
		std::size_t InsertionFileOffset;
		std::size_t InsertedTokenCount;
		std::size_t OriginalTempCount;
	};

	class DxbcLambertGuardPatcher final
	{
	public:
		auto Patch(const DxbcContainerView& container, const LambertShaderIdentity& identity) const
			-> std::expected<DxbcLambertGuardPatchResult, DxbcError>;
	};
}
