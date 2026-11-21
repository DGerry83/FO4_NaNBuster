#pragma once

#include <expected>
#include <span>

#include "Core/DxbcContainer.h"

namespace NaNBuster
{
	// Finalizes the 16-byte embedded hash of a DXBC container in place.
	// Write validates the container with DxbcInspector, recomputes the
	// checksum over bytes [20, size) using the d3dcompiler embedded-hash
	// convention (MD5 compression with a custom final block; payload starts
	// at offset 20, so the existing hash field is excluded, not zeroed), and
	// writes the digest to bytes [4, 20). Any other byte is left unchanged;
	// a rejected input is returned with the inspector's error and the input
	// bytes untouched. Works on any container DxbcInspector accepts.
	// Standard-library only; checked arithmetic and byte-wise reads/writes
	// only; never throws.
	class DxbcHashWriter
	{
	public:
		auto Write(std::span<std::byte> correctedBytes) const
			-> std::expected<void, DxbcError>;
	};
}
