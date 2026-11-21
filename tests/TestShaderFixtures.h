#pragma once

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Core/ShaderIdentity.h"

#ifndef NANBUSTER_ORIGINAL_SHADER_PATH
#	error "NANBUSTER_ORIGINAL_SHADER_PATH must be defined by the test target (absolute path to the read-only original DXBC)."
#endif

namespace NaNBuster::TestFixtures
{
	// Reads an entire binary file into caller-owned storage.
	// Throws std::runtime_error on open, size, seek, or read failure.
	inline auto LoadBinaryFile(const std::string& path) -> std::vector<std::byte>
	{
		std::ifstream stream(path, std::ios::binary | std::ios::ate);
		if (!stream.is_open()) {
			throw std::runtime_error("TestShaderFixtures: failed to open '" + path + "'.");
		}

		const auto size = static_cast<std::streamoff>(stream.tellg());
		if (size < 0) {
			throw std::runtime_error("TestShaderFixtures: failed to determine size of '" + path + "'.");
		}

		std::vector<std::byte> bytes(static_cast<std::size_t>(size));
		stream.seekg(0, std::ios::beg);
		if (!stream || (size > 0 && !stream.read(reinterpret_cast<char*>(bytes.data()), size))) {
			throw std::runtime_error("TestShaderFixtures: failed to read '" + path + "'.");
		}

		return bytes;
	}

	// Loads the read-only original target DXBC. The returned vector owns the
	// storage; every ByteView into it must not outlive the returned vector.
	inline auto LoadOriginalShader() -> std::vector<std::byte>
	{
		return LoadBinaryFile(NANBUSTER_ORIGINAL_SHADER_PATH);
	}

#ifdef NANBUSTER_CORRECTED_SHADER_PATH
	// Loads the generated corrected DXBC build artifact. Available only to test
	// targets that define NANBUSTER_CORRECTED_SHADER_PATH (Infrastructure.Tests);
	// Core.Tests compilation is unchanged. The returned vector owns the storage.
	inline auto LoadCorrectedShader() -> std::vector<std::byte>
	{
		return LoadBinaryFile(NANBUSTER_CORRECTED_SHADER_PATH);
	}
#endif

	// Immutable non-owning view over caller-owned storage.
	inline auto AsByteView(const std::vector<std::byte>& bytes) -> ByteView
	{
		return ByteView{ bytes.data(), bytes.size() };
	}
}
