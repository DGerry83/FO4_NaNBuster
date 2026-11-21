#include "Core/DxbcInspector.h"
#include "TestShaderFixtures.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "patched.inc" // generated data-only declarations (NaNBuster::Generated)

#include <d3d11.h>

namespace
{
	// Independently proven corrected-container values (G3_CLAMP_EVIDENCE.md,
	// G3_HASH_EVIDENCE.md). The full corrected SHA-256 is recorded offline in
	// G3_OFFLINE_VALIDATION.md and asserted here after an in-test SHA-256.
	constexpr std::size_t ExpectedOriginalSize = 1612;
	constexpr std::size_t ExpectedCorrectedSize = 1652;
	constexpr std::size_t DxbcEmbeddedHashOffset = 4;
	constexpr std::array<std::byte, 16> ExpectedCorrectedEmbeddedHash = {
		std::byte{ 0xc6 }, std::byte{ 0xe8 }, std::byte{ 0xde }, std::byte{ 0x1d },
		std::byte{ 0xd3 }, std::byte{ 0x01 }, std::byte{ 0x68 }, std::byte{ 0xfb },
		std::byte{ 0x0b }, std::byte{ 0x4b }, std::byte{ 0xd6 }, std::byte{ 0xeb },
		std::byte{ 0x9b }, std::byte{ 0xa1 }, std::byte{ 0x09 }, std::byte{ 0xfa },
	};
	constexpr std::string_view ExpectedCorrectedSha256 = "9387444bcc52952a020a0414fac752c24c4155c7b8b84e9d030d9ca3c6542fae";

	// Disassembly statement lines include the ps_5_0 version line; the
	// inspector instruction count excludes it (version is not an instruction).
	constexpr std::size_t ExpectedOriginalAsmStatements = 55;
	constexpr std::size_t ExpectedCorrectedAsmStatements = 56;
	constexpr std::size_t ExpectedCorrectedInstructions = 55;
	constexpr std::string_view OriginalLogLine = "log r0.xyzw, v2.xyzw";
	constexpr std::string_view CorrectedLogLine = "log r0.xyzw, r0.xyzw";
	constexpr std::string_view InsertedMaxLine = "max r0.xyzw, v2.xyzw, l(0.000000, 0.000000, 0.000000, 0.000000)";

	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}

	// Minimal FIPS 180-4 SHA-256 over a byte range (standard C++, no external
	// dependency). Exists so the full corrected-file digest is asserted in-test
	// against the independently recorded offline digest.
	class Sha256
	{
	public:
		Sha256() { m_h = { 0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u }; }

		void Update(const std::uint8_t* data, std::size_t size)
		{
			m_total += size;
			while (size > 0) {
				const std::size_t take = (m_bufferSize < 64) ? (64 - m_bufferSize) : 64;
				const std::size_t n = take < size ? take : size;
				std::memcpy(m_buffer.data() + m_bufferSize, data, n);
				m_bufferSize += n;
				data += n;
				size -= n;
				if (m_bufferSize == 64) {
					Transform(m_buffer.data());
					m_bufferSize = 0;
				}
			}
		}

		auto Finalize() -> std::array<std::uint8_t, 32>
		{
			const std::uint64_t bitCount = m_total * 8;
			const std::uint8_t marker = 0x80;
			Update(&marker, 1);
			const std::uint8_t zero = 0;
			while (m_bufferSize != 56) {
				Update(&zero, 1);
			}
			std::uint8_t lengthBytes[8];
			for (std::size_t i = 0; i < 8; ++i) {
				lengthBytes[i] = static_cast<std::uint8_t>((bitCount >> (56 - i * 8)) & 0xFF);
			}
			// Feed length directly without touching m_total accounting.
			std::memcpy(m_buffer.data() + 56, lengthBytes, 8);
			Transform(m_buffer.data());
			std::array<std::uint8_t, 32> digest{};
			for (std::size_t i = 0; i < 8; ++i) {
				digest[i * 4] = static_cast<std::uint8_t>(m_h[i] >> 24);
				digest[i * 4 + 1] = static_cast<std::uint8_t>(m_h[i] >> 16);
				digest[i * 4 + 2] = static_cast<std::uint8_t>(m_h[i] >> 8);
				digest[i * 4 + 3] = static_cast<std::uint8_t>(m_h[i]);
			}
			return digest;
		}

	private:
		static auto RotateRight(std::uint32_t value, std::uint32_t bits) -> std::uint32_t
		{
			return (value >> bits) | (value << (32 - bits));
		}

		void Transform(const std::uint8_t* block)
		{
			static constexpr std::uint32_t K[64] = {
				0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
				0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
				0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
				0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
				0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
				0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
				0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
				0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
			};
			std::uint32_t w[64];
			for (std::size_t i = 0; i < 16; ++i) {
				w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
					(static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
					(static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
					static_cast<std::uint32_t>(block[i * 4 + 3]);
			}
			for (std::size_t i = 16; i < 64; ++i) {
				const std::uint32_t s0 = RotateRight(w[i - 15], 7) ^ RotateRight(w[i - 15], 18) ^ (w[i - 15] >> 3);
				const std::uint32_t s1 = RotateRight(w[i - 2], 17) ^ RotateRight(w[i - 2], 19) ^ (w[i - 2] >> 10);
				w[i] = w[i - 16] + s0 + w[i - 7] + s1;
			}
			std::uint32_t a = m_h[0];
			std::uint32_t b = m_h[1];
			std::uint32_t c = m_h[2];
			std::uint32_t d = m_h[3];
			std::uint32_t e = m_h[4];
			std::uint32_t f = m_h[5];
			std::uint32_t g = m_h[6];
			std::uint32_t h = m_h[7];
			for (std::size_t i = 0; i < 64; ++i) {
				const std::uint32_t s1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
				const std::uint32_t ch = (e & f) ^ (~e & g);
				const std::uint32_t temp1 = h + s1 + ch + K[i] + w[i];
				const std::uint32_t s0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
				const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
				const std::uint32_t temp2 = s0 + maj;
				h = g;
				g = f;
				f = e;
				e = d + temp1;
				d = c;
				c = b;
				b = a;
				a = temp1 + temp2;
			}
			m_h[0] += a;
			m_h[1] += b;
			m_h[2] += c;
			m_h[3] += d;
			m_h[4] += e;
			m_h[5] += f;
			m_h[6] += g;
			m_h[7] += h;
		}

		std::array<std::uint32_t, 8> m_h{};
		std::array<std::uint8_t, 64> m_buffer{};
		std::size_t m_bufferSize = 0;
		std::uint64_t m_total = 0;
	};

	auto HexDigest(const std::array<std::uint8_t, 32>& digest) -> std::string
	{
		static constexpr char digits[] = "0123456789abcdef";
		std::string out;
		out.reserve(digest.size() * 2);
		for (const std::uint8_t byte : digest) {
			out += digits[byte >> 4];
			out += digits[byte & 0x0F];
		}
		return out;
	}

	auto ReadLines(const std::string& path) -> std::vector<std::string>
	{
		std::ifstream stream(path);
		if (!stream.is_open()) {
			throw std::runtime_error("failed to open '" + path + "'");
		}
		std::vector<std::string> lines;
		std::string line;
		while (std::getline(stream, line)) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
				line.pop_back();
			}
			if (line.empty() || line.rfind("//", 0) == 0) {
				continue;
			}
			lines.push_back(line);
		}
		return lines;
	}

	struct ReleaseDeleter
	{
		template <typename T>
		void operator()(T* pointer) const
		{
			if (pointer != nullptr) {
				pointer->Release();
			}
		}
	};

	// Emulates the corrected shader semantics for one channel: the inserted
	// max against zero, then the existing log/mul/exp sequence computing
	// pow(clamped, 2.2) as exp2(2.2 * log2(clamped)).
	auto ShaderChannelValue(float input) -> float
	{
		const float clamped = input > 0.0f ? input : 0.0f;
		if (clamped == 0.0f) {
			return 0.0f;
		}
		return std::exp2(2.2f * std::log2(clamped));
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

static_assert(NaNBuster::Generated::CorrectedShaderDxbc.size() == ExpectedCorrectedSize);
static_assert(NaNBuster::Generated::CorrectedShaderDxbcSize == ExpectedCorrectedSize);

auto main() -> int
{
	// ---- Fixture loading (RAII file handles inside fixtures) ----
	const std::vector<std::byte> original = NaNBuster::TestFixtures::LoadOriginalShader();
#ifdef NANBUSTER_CORRECTED_SHADER_PATH
	const std::vector<std::byte> corrected = NaNBuster::TestFixtures::LoadCorrectedShader();
#endif
	const NaNBuster::ByteView originalView = NaNBuster::TestFixtures::AsByteView(original);
	const NaNBuster::ByteView correctedView = NaNBuster::TestFixtures::AsByteView(corrected);

	// ---- Corrected file integrity: size, embedded hash, full SHA-256 ----
	CHECK(originalView.size() == ExpectedOriginalSize);
	CHECK(correctedView.size() == ExpectedCorrectedSize);
	CHECK(correctedView.size() >= DxbcEmbeddedHashOffset + ExpectedCorrectedEmbeddedHash.size());
	CHECK(std::memcmp(
			  correctedView.data() + DxbcEmbeddedHashOffset,
			  ExpectedCorrectedEmbeddedHash.data(),
			  ExpectedCorrectedEmbeddedHash.size()) == 0);

	{
		Sha256 sha256;
		sha256.Update(reinterpret_cast<const std::uint8_t*>(correctedView.data()), correctedView.size());
		const std::string digest = HexDigest(sha256.Finalize());
		CHECK(digest == ExpectedCorrectedSha256);
		if (digest != ExpectedCorrectedSha256) {
			std::cerr << "corrected SHA-256: " << digest << '\n';
		}
	}

	// ---- Generated-include integrity: array equals patched.dxbc byte-for-byte ----
	CHECK(NaNBuster::Generated::CorrectedShaderDxbc.size() == correctedView.size());
	CHECK(NaNBuster::Generated::CorrectedShaderDxbcSize == correctedView.size());
	CHECK(std::equal(
		NaNBuster::Generated::CorrectedShaderDxbc.begin(),
		NaNBuster::Generated::CorrectedShaderDxbc.end(),
		reinterpret_cast<const std::uint8_t*>(correctedView.data())));

	// ---- Container re-inspection of the corrected bytes ----
	const NaNBuster::DxbcInspector inspector;
	auto reinspected = inspector.Inspect(correctedView);
	CHECK(reinspected.has_value());
	if (reinspected) {
		CHECK(reinspected->Chunks().size() == 3);
		CHECK(reinspected->Instructions().size() == ExpectedCorrectedInstructions);
		CHECK(reinspected->ShaderTokenCount() == 344);
		CHECK(!reinspected->TargetRegion().has_value());
	}

	// ---- Disassembly diff: patched.asm vs the read-only original.asm ----
	try {
		const std::vector<std::string> originalLines = ReadLines(NANBUSTER_ORIGINAL_ASM_PATH);
		const std::vector<std::string> correctedLines = ReadLines(NANBUSTER_PATCHED_ASM_PATH);

		CHECK(originalLines.size() == ExpectedOriginalAsmStatements);
		CHECK(correctedLines.size() == ExpectedCorrectedAsmStatements);

		std::size_t logIndex = originalLines.size();
		for (std::size_t i = 0; i < originalLines.size(); ++i) {
			if (originalLines[i] == OriginalLogLine) {
				logIndex = i;
				break;
			}
		}
		CHECK(logIndex != originalLines.size());

		if (logIndex != originalLines.size() &&
			correctedLines.size() == ExpectedCorrectedAsmStatements &&
			originalLines.size() == ExpectedOriginalAsmStatements) {
			CHECK(correctedLines[logIndex] == InsertedMaxLine);
			CHECK(correctedLines[logIndex + 1] == CorrectedLogLine);
			bool identicalElsewhere = true;
			for (std::size_t i = 0; i < correctedLines.size(); ++i) {
				if (i == logIndex || i == logIndex + 1) {
					continue; // anchors asserted above: inserted max + rewritten log
				}
				const std::string& expected = (i < logIndex) ? originalLines[i] : originalLines[i - 1];
				if (correctedLines[i] != expected) {
					identicalElsewhere = false;
					std::cerr << "disassembly divergence at statement " << i << ": '" << correctedLines[i]
							  << "' vs expected '" << expected << "'\n";
					break;
				}
			}
			CHECK(identicalElsewhere);
		}

		std::size_t maxCount = 0;
		for (const std::string& line : correctedLines) {
			if (line.rfind("max ", 0) == 0) {
				++maxCount;
			}
		}
		CHECK(maxCount == 1);
	} catch (const std::exception& error) {
		CHECK(!"failed to read disassembly inputs");
		std::cerr << "disassembly read error: " << error.what() << '\n';
	}

	// ---- D3D11 CreatePixelShader: original and corrected (WARP and/or hardware) ----
	{
		std::unique_ptr<ID3D11Device, ReleaseDeleter> device;
		D3D_DRIVER_TYPE driverTypeUsed = D3D_DRIVER_TYPE_NULL;
		HRESULT deviceResult = E_FAIL;
		for (D3D_DRIVER_TYPE type : { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP }) {
			ID3D11Device* rawDevice = nullptr;
			deviceResult = D3D11CreateDevice(
				nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
				&rawDevice, nullptr, nullptr);
			if (SUCCEEDED(deviceResult)) {
				device.reset(rawDevice);
				driverTypeUsed = type;
				break;
			}
		}
		CHECK(SUCCEEDED(deviceResult));
		if (device) {
			ID3D11PixelShader* rawOriginal = nullptr;
			const HRESULT originalResult = device->CreatePixelShader(
				originalView.data(), originalView.size(), nullptr, &rawOriginal);
			std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> originalShader(rawOriginal);
			CHECK(SUCCEEDED(originalResult));

			ID3D11PixelShader* rawCorrected = nullptr;
			const HRESULT correctedResult = device->CreatePixelShader(
				correctedView.data(), correctedView.size(), nullptr, &rawCorrected);
			std::unique_ptr<ID3D11PixelShader, ReleaseDeleter> correctedShader(rawCorrected);
			CHECK(SUCCEEDED(correctedResult));

			const char* driverName = driverTypeUsed == D3D_DRIVER_TYPE_HARDWARE ? "hardware" : "WARP";
			std::cout << "D3D11 CreatePixelShader: device=" << driverName
					  << " original=0x" << std::hex << static_cast<unsigned long>(originalResult)
					  << " corrected=0x" << static_cast<unsigned long>(correctedResult) << std::dec << '\n';
		}
	}

	// ---- Four-channel math vectors for the inserted clamp ----
	{
		constexpr float kInputs[] = { -2.0f, -0.25f, 0.0f, 0.1f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 8.0f };
		for (const float input : kInputs) {
			const float expected = input > 0.0f ? static_cast<float>(std::pow(static_cast<double>(input), 2.2)) : 0.0f;
			std::array<float, 4> channels = { input, input, input, input };
			std::array<float, 4> results = {};
			for (std::size_t channel = 0; channel < channels.size(); ++channel) {
				results[channel] = ShaderChannelValue(channels[channel]);
			}
			for (std::size_t channel = 0; channel < results.size(); ++channel) {
				CHECK(std::isfinite(results[channel]));
				if (input <= 0.0f) {
					CHECK(results[channel] == 0.0f);
					CHECK(!std::signbit(results[channel]));
				} else {
					CHECK(std::fabs(results[channel] - expected) <= 1e-4f * (expected + 1e-6f));
				}
			}
		}

		// Per-channel mixed vector exercises all four COLOR0 lanes at once.
		const std::array<float, 4> mixed = { -0.5f, 0.0f, 0.5f, 2.0f };
		const std::array<float, 4> mixedExpected = {
			0.0f,
			0.0f,
			static_cast<float>(std::pow(0.5, 2.2)),
			static_cast<float>(std::pow(2.0, 2.2)),
		};
		for (std::size_t channel = 0; channel < mixed.size(); ++channel) {
			const float result = ShaderChannelValue(mixed[channel]);
			CHECK(std::isfinite(result));
			CHECK(std::fabs(result - mixedExpected[channel]) <= 1e-4f * (mixedExpected[channel] + 1e-6f));
		}

		// Vulnerability witness: without the clamp, a negative COLOR0 lane
		// makes log2 negative, i.e. NaN after mul/exp.
		CHECK(std::isnan(std::log2(-0.5f)));
	}

	if (g_failures != 0) {
		std::cerr << "Infrastructure.CorrectedShaderValidation: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Infrastructure.CorrectedShaderValidation: all checks passed (corrected "
			  << correctedView.size() << " bytes).\n";
	return EXIT_SUCCESS;
}
