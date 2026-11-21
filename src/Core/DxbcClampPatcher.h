#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <vector>

#include "Core/DxbcContainer.h"

namespace NaNBuster
{
	// Owned result of a successful clamp transformation. Bytes is the sole
	// owned corrected storage; all metadata is numeric so the result stays
	// valid after moves and after the input view and its storage are gone.
	struct DxbcPatchResult
	{
		std::vector<std::byte> Bytes;
		std::size_t InsertionTokenOffset; // DWORD index of the inserted MAX in the corrected stream
		std::size_t InsertionFileOffset;  // absolute container byte offset of the inserted MAX
		std::size_t LogTokenOffset;       // DWORD index of the rewritten LOG in the corrected stream
		std::size_t LogFileOffset;        // absolute container byte offset of the rewritten LOG
		std::size_t InsertedByteCount;    // bytes added to the container (40)
	};

	// Transforms a validated view of the exact original target into the
	// corrected container: inserts max r0.xyzw, v2.xyzw, l(0,0,0,0)
	// immediately before the validated LOG and rewrites only the LOG source
	// operand from v2.xyzw to r0.xyzw. Requires original size 1612, the exact
	// embedded hash at offset 4, and the validated target region; an already
	// clamped stream rejects with AlreadyPatched. Standard-library only;
	// checked arithmetic and byte-wise reads/writes only; never throws.
	class DxbcClampPatcher
	{
	public:
		auto Patch(const DxbcContainerView& container) const
			-> std::expected<DxbcPatchResult, DxbcError>;
	};
}
