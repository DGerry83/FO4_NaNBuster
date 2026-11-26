#pragma once

#include "Core/DxbcContainer.h"

#include <cstddef>
#include <expected>
#include <vector>

namespace NaNBuster
{
	struct DxbcIsotropicSpecularAAPatchResult
	{
		std::vector<std::byte> Bytes;
		std::size_t VarianceInsertionTokenOffset;
		std::size_t VarianceInsertionFileOffset;
		std::size_t ExponentOriginalTokenOffset;
		std::size_t ExponentOriginalFileOffset;
		std::size_t InsertedTokenCount;
		std::size_t OriginalTempCount;
	};

	class DxbcIsotropicSpecularAAPatcher final
	{
	public:
		auto Patch(const DxbcContainerView& container) const
			-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>;
	};
}
