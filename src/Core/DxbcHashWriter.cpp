#include "Core/DxbcHashWriter.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "Core/DxbcInspector.h"

namespace NaNBuster
{
	namespace
	{
		constexpr std::size_t kHashOffset = 4;
		constexpr std::size_t kPayloadOffset = 20; // magic4 + hash16
		constexpr std::size_t kDigestSize = 16;
		constexpr std::size_t kMd5BlockSize = 64;

		// MD5 per RFC 1321 (round constants and per-round left-rotate counts).
		constexpr std::array<std::uint32_t, 64> kMd5K = {
			0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
			0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
			0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
			0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
			0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
			0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
			0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
			0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
			0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
			0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
			0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
			0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
			0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
			0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
			0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
			0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u,
		};

		constexpr std::array<std::uint32_t, 64> kMd5S = {
			7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
			5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
			4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
			6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
		};

		struct Md5State
		{
			std::uint32_t A;
			std::uint32_t B;
			std::uint32_t C;
			std::uint32_t D;
		};

		auto LoadLe32(std::span<const std::byte, 4> bytes) -> std::uint32_t
		{
			std::uint32_t value = 0;
			for (std::size_t i = 0; i < 4; ++i) {
				value |= static_cast<std::uint32_t>(bytes[i]) << (8 * i);
			}
			return value;
		}

		void StoreLe32(std::span<std::byte, 4> out, std::uint32_t value)
		{
			for (std::size_t i = 0; i < 4; ++i) {
				out[i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
			}
		}

		void ProcessBlock(Md5State& state, std::span<const std::byte, kMd5BlockSize> block)
		{
			std::uint32_t m[16];
			for (std::size_t i = 0; i < 16; ++i) {
				m[i] = LoadLe32(std::span<const std::byte, 4>{ block.data() + i * 4, 4 });
			}

			std::uint32_t a = state.A;
			std::uint32_t b = state.B;
			std::uint32_t c = state.C;
			std::uint32_t d = state.D;

			for (std::size_t i = 0; i < 64; ++i) {
				std::uint32_t f;
				std::size_t g;
				if (i < 16) {
					f = (b & c) | (~b & d);
					g = i;
				} else if (i < 32) {
					f = (d & b) | (~d & c);
					g = (5 * i + 1) % 16;
				} else if (i < 48) {
					f = b ^ c ^ d;
					g = (3 * i + 5) % 16;
				} else {
					f = c ^ (b | ~d);
					g = (7 * i) % 16;
				}

				const std::uint32_t sum = f + a + kMd5K[i] + m[g];
				a = d;
				d = c;
				c = b;
				b = b + std::rotl(sum, static_cast<int>(kMd5S[i]));
			}

			state.A += a;
			state.B += b;
			state.C += c;
			state.D += d;
		}

		// Computes the d3dcompiler DXBC embedded checksum over payload =
		// container bytes [20, size): standard MD5 block processing, then a
		// custom final block instead of RFC 1321 padding (RenderDoc
		// dxbc_container.cpp / vkd3d-proton checksum.c; see
		// G3_HASH_EVIDENCE.md section 1 for the full citation and tail layout).
		auto ComputeDxbcChecksum(std::span<const std::byte> payload) -> std::array<std::byte, kDigestSize>
		{
			const std::uint32_t numBits = static_cast<std::uint32_t>(payload.size() * 8u);
			const std::uint32_t numBits2 = (numBits >> 2) | 1u;

			Md5State state{ 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };

			const std::size_t leftover = payload.size() % kMd5BlockSize;
			const std::size_t bulkLength = payload.size() - leftover;
			for (std::size_t offset = 0; offset < bulkLength; offset += kMd5BlockSize) {
				ProcessBlock(state, std::span<const std::byte, kMd5BlockSize>{ payload.data() + offset, kMd5BlockSize });
			}

			const std::byte* tail = payload.data() + bulkLength;
			std::array<std::byte, kMd5BlockSize> block{};

			if (leftover >= 56) {
				// [leftover data][0x80 + zeros to block end], then a final
				// block of numBits at [0,4) and numBits2 at [60,64).
				for (std::size_t i = 0; i < leftover; ++i) {
					block[i] = tail[i];
				}
				block[leftover] = std::byte{ 0x80 };
				ProcessBlock(state, std::span<const std::byte, kMd5BlockSize>{ block.data(), kMd5BlockSize });

				block = {};
				StoreLe32(std::span<std::byte, 4>{ block.data(), 4 }, numBits);
				StoreLe32(std::span<std::byte, 4>{ block.data() + 60, 4 }, numBits2);
				ProcessBlock(state, std::span<const std::byte, kMd5BlockSize>{ block.data(), kMd5BlockSize });
			} else {
				// One final block: numBits FIRST at [0,4), then the leftover
				// data, then 0x80, zeros, and numBits2 at [60,64).
				StoreLe32(std::span<std::byte, 4>{ block.data(), 4 }, numBits);
				for (std::size_t i = 0; i < leftover; ++i) {
					block[4 + i] = tail[i];
				}
				block[4 + leftover] = std::byte{ 0x80 };
				StoreLe32(std::span<std::byte, 4>{ block.data() + 60, 4 }, numBits2);
				ProcessBlock(state, std::span<const std::byte, kMd5BlockSize>{ block.data(), kMd5BlockSize });
			}

			std::array<std::byte, kDigestSize> digest{};
			StoreLe32(std::span<std::byte, 4>{ digest.data(), 4 }, state.A);
			StoreLe32(std::span<std::byte, 4>{ digest.data() + 4, 4 }, state.B);
			StoreLe32(std::span<std::byte, 4>{ digest.data() + 8, 4 }, state.C);
			StoreLe32(std::span<std::byte, 4>{ digest.data() + 12, 4 }, state.D);
			return digest;
		}
	}

	auto DxbcHashWriter::Write(std::span<std::byte> correctedBytes) const
		-> std::expected<void, DxbcError>
	{
		try {
			const DxbcInspector inspector;
			auto inspected = inspector.Inspect(ByteView{ correctedBytes.data(), correctedBytes.size() });
			if (!inspected.has_value()) {
				return std::unexpected(inspected.error());
			}

			const std::size_t size = correctedBytes.size();
			if (size < kPayloadOffset ||
				size - kPayloadOffset > std::numeric_limits<std::uint32_t>::max() / 8u) {
				return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, kPayloadOffset });
			}

			const std::span<const std::byte> payload{ correctedBytes.data() + kPayloadOffset, size - kPayloadOffset };
			const auto digest = ComputeDxbcChecksum(payload);

			for (std::size_t i = 0; i < kDigestSize; ++i) {
				correctedBytes[kHashOffset + i] = digest[i];
			}
			return {};
		} catch (...) {
			return std::unexpected(DxbcError{ DxbcErrorCode::InternalError, kPayloadOffset });
		}
	}
}
