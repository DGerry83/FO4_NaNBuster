#pragma once

#include "Core/DxbcContainer.h"
#include "Core/LambertShaderIdentities.h"

#include <cstddef>
#include <expected>
#include <vector>

namespace NaNBuster
{
	struct DxbcAnisotropicSpecularClampPatchResult
	{
		std::vector<std::byte> Bytes;
		std::size_t InsertionTokenOffset;
		std::size_t InsertionFileOffset;
		std::size_t InsertedTokenCount;
		std::size_t OriginalTempCount;
		bool IncludesLambertGuard;
	};

	class DxbcAnisotropicSpecularClampPatcher final
	{
	public:
		auto Patch(
			const DxbcContainerView& container,
			const LambertShaderIdentity& identity,
			bool includeLambertGuard = false) const
			-> std::expected<DxbcAnisotropicSpecularClampPatchResult, DxbcError>;
	};
}
