#pragma once

#include "Core/DxbcContainer.h"
#include "Core/ShaderIdentity.h"

#include <array>

#include <cstddef>
#include <expected>
#include <vector>

namespace NaNBuster
{
struct IsotropicSpecularAAVariantLayout
{
	std::size_t CullTokenOffset;
	std::array<std::size_t, 6> DecodeTokenOffsets;
	std::size_t ExponentMadTokenOffset;
	std::size_t ExponentTokenOffset;
	std::size_t ExponentInsertionTokenOffset;
	std::uint32_t ExponentRegister;
	std::uint32_t ExponentComponent;
};

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
	auto Patch(const DxbcContainerView &container) const
		-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>;
	auto Patch(const DxbcContainerView &container, const ShaderIdentity &identity) const
		-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>;
	auto PatchVariant(const DxbcContainerView &container, const ShaderIdentity &identity,
					  const IsotropicSpecularAAVariantLayout &layout) const
		-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>;

  private:
	auto PatchWithLayout(const DxbcContainerView &container, const ShaderIdentity &identity,
						 const IsotropicSpecularAAVariantLayout *layout) const
		-> std::expected<DxbcIsotropicSpecularAAPatchResult, DxbcError>;
};
} // namespace NaNBuster
