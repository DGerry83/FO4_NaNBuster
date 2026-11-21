#include "Core/DxbcContainer.h"
#include "Core/DxbcInspector.h"
#include "TestShaderFixtures.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	using NaNBuster::ByteView;
	using NaNBuster::DxbcContainerView;
	using NaNBuster::DxbcError;
	using NaNBuster::DxbcErrorCode;
	using NaNBuster::DxbcInspector;

	int g_failures = 0;

	void Check(bool condition, const char* expression, const char* file, int line)
	{
		if (!condition) {
			++g_failures;
			std::cerr << "FAILED: " << expression << " (" << file << ':' << line << ")\n";
		}
	}
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

namespace
{
	using Bytes = std::vector<std::byte>;

	void PutU32(Bytes& bytes, std::uint32_t value)
	{
		for (unsigned shift = 0; shift < 32; shift += 8) {
			bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFu));
		}
	}

	void SetU32(Bytes& bytes, std::size_t offset, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) {
			bytes[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
		}
	}

	std::uint32_t GetU32(const Bytes& bytes, std::size_t offset)
	{
		std::uint32_t value = 0;
		for (unsigned i = 0; i < 4; ++i) {
			value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
		}
		return value;
	}

	struct ChunkSpec
	{
		char FourCC[4];
		Bytes Content;
	};

	// Lays chunks out consecutively after the chunk-offset table.
	auto BuildContainer(const std::vector<ChunkSpec>& chunks) -> Bytes
	{
		Bytes bytes;
		const std::uint32_t tableSize = static_cast<std::uint32_t>(chunks.size()) * 4u;
		bytes.insert(bytes.end(), { std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ 'B' }, std::byte{ 'C' } });
		for (int i = 0; i < 16; ++i) {
			bytes.push_back(std::byte{ 0 });
		}
		PutU32(bytes, 1u); // version
		const std::size_t totalOffset = bytes.size();
		PutU32(bytes, 0u); // patched below
		PutU32(bytes, static_cast<std::uint32_t>(chunks.size()));

		std::uint32_t chunkOffset = 32u + tableSize;
		std::vector<std::uint32_t> offsets;
		Bytes body;
		for (const ChunkSpec& chunk : chunks) {
			offsets.push_back(chunkOffset);
			for (char c : chunk.FourCC) {
				body.push_back(static_cast<std::byte>(c));
			}
			PutU32(body, static_cast<std::uint32_t>(chunk.Content.size()));
			body.insert(body.end(), chunk.Content.begin(), chunk.Content.end());
			chunkOffset += 8u + static_cast<std::uint32_t>(chunk.Content.size());
		}
		for (std::uint32_t offset : offsets) {
			PutU32(bytes, offset);
		}
		bytes.insert(bytes.end(), body.begin(), body.end());
		SetU32(bytes, totalOffset, static_cast<std::uint32_t>(bytes.size()));
		return bytes;
	}

	auto MakeShexChunk(const std::vector<std::uint32_t>& tokens) -> ChunkSpec
	{
		ChunkSpec chunk{ { 'S', 'H', 'E', 'X' }, {} };
		PutU32(chunk.Content, 0x50u); // ps_5_0
		PutU32(chunk.Content, static_cast<std::uint32_t>(tokens.size()) + 2u);
		for (std::uint32_t token : tokens) {
			PutU32(chunk.Content, token);
		}
		return chunk;
	}

	auto MakeShexChunkWithContent(Bytes content) -> ChunkSpec
	{
		return ChunkSpec{ { 'S', 'H', 'E', 'X' }, std::move(content) };
	}

	auto MakeIsgnChunk() -> ChunkSpec
	{
		ChunkSpec chunk{ { 'I', 'S', 'G', 'N' }, {} };
		for (int i = 0; i < 16; ++i) {
			chunk.Content.push_back(std::byte{ 0x5A });
		}
		return chunk;
	}

	constexpr std::uint32_t kDclTemps3 = 0x02000068u; // dcl_temps 3
	constexpr std::uint32_t kRet = 0x0100003Eu;       // ret (opcode 62, len 1)
	constexpr std::uint32_t kLogR0V2 = 0x0500002Fu;   // log r0.xyzw, v2.xyzw
	constexpr std::uint32_t kMulR0Gamma = 0x0A000038u; // mul r0.xyzw, r0.xyzw, l(2.2 x4)
	constexpr std::uint32_t kExpR0 = 0x05000019u;     // exp r0.xyzw, r0.xyzw

	auto GammaImmediateTokens() -> std::vector<std::uint32_t>
	{
		return {
			0x001000F2u, 0x00000000u, // dest r0.xyzw
			0x00100E46u, 0x00000000u, // src r0.xyzw
			0x00004002u,              // imm32, 4 components
			0x400CCCCDu, 0x400CCCCDu, 0x400CCCCDu, 0x400CCCCDu, // 2.2f x4
		};
	}

	auto InspectError(const Bytes& bytes) -> DxbcError
	{
		const DxbcInspector inspector;
		auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
		if (result.has_value()) {
			return DxbcError{ DxbcErrorCode::InternalError, 0xFFFFFFFFu };
		}
		return result.error();
	}

	void ExpectError(const Bytes& bytes, DxbcErrorCode code, std::size_t offset, const char* file, int line)
	{
		const DxbcInspector inspector;
		auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
		Check(!result.has_value(), "malformed input accepted", file, line);
		if (!result.has_value()) {
			Check(result.error().Code == code, "error code", file, line);
			Check(result.error().Offset == offset, "error offset", file, line);
			if (result.error().Code != code || result.error().Offset != offset) {
				std::cerr << "  expected code=" << static_cast<int>(code) << " offset=" << offset
					<< " got code=" << static_cast<int>(result.error().Code)
					<< " offset=" << result.error().Offset << '\n';
			}
		}
	}

#define EXPECT_ERROR(bytes, code, offset) ExpectError((bytes), (code), (offset), __FILE__, __LINE__)

	// ---- Original oracle tests ----

	void OriginalOracleTests()
	{
		auto original = NaNBuster::TestFixtures::LoadOriginalShader();
		const ByteView view = NaNBuster::TestFixtures::AsByteView(original);

		const DxbcInspector inspector;
		auto result = inspector.Inspect(view);
		CHECK(result.has_value());
		if (!result.has_value()) {
			return;
		}
		const DxbcContainerView& container = *result;

		CHECK(container.OriginalBytes().size() == 1612u);

		// Declared container size and chunk layout.
		CHECK(container.Chunks().size() == 3u);
		CHECK(container.Chunks()[0].FourCC == 0x4E475349u); // "ISGN"
		CHECK(container.Chunks()[0].Offset == 44u);
		CHECK(container.Chunks()[0].Size == 156u);
		CHECK(container.Chunks()[1].FourCC == 0x4E47534Fu); // "OSGN"
		CHECK(container.Chunks()[1].Offset == 208u);
		CHECK(container.Chunks()[1].Size == 44u);
		CHECK(container.Chunks()[2].FourCC == 0x58454853u); // "SHEX"
		CHECK(container.Chunks()[2].Offset == 260u);
		CHECK(container.Chunks()[2].Size == 1344u);

		// Every chunk payload stays inside the original bytes.
		for (std::size_t i = 0; i < container.Chunks().size(); ++i) {
			const ByteView payload = container.ChunkPayload(i);
			CHECK(payload.size() == container.Chunks()[i].Size);
			CHECK(payload.data() >= container.OriginalBytes().data());
			CHECK(payload.data() + payload.size() <=
				container.OriginalBytes().data() + container.OriginalBytes().size());
		}

		// Exactly one shader chunk and a complete instruction stream.
		std::size_t shaderChunks = 0;
		for (const auto& chunk : container.Chunks()) {
			if (chunk.FourCC == 0x58454853u || chunk.FourCC == 0x52444853u) {
				++shaderChunks;
			}
		}
		CHECK(shaderChunks == 1u);
		CHECK(container.ShaderStreamFileOffset() == 0x114u);
		CHECK(container.ShaderTokenCount() == 334u);
		CHECK(container.Instructions().size() == 54u);
		std::size_t consumed = 0;
		for (const auto& instruction : container.Instructions()) {
			consumed += instruction.TokenLength;
		}
		CHECK(consumed == 334u);
		CHECK(container.Instructions().front().Opcode == 106u); // dcl_globalFlags
		CHECK(container.Instructions().back().Opcode == 62u);   // ret

		// Exact vulnerable log/mul/exp region with four-channel v2 source.
		CHECK(container.TargetRegion().has_value());
		if (container.TargetRegion().has_value()) {
			const auto& region = *container.TargetRegion();
			CHECK(region.TokenOffset == 45u);
			CHECK(region.TokenLength == 5u);
			CHECK(region.DestinationRegister == 0u);
			CHECK(region.SourceRegister == 2u);
			CHECK(region.ChannelMask == 0x0Fu);
			CHECK(container.ShaderStreamFileOffset() + region.TokenOffset * 4u == 0x1C8u);
		}
		CHECK(container.Instructions()[14].TokenOffset == 45u);
		CHECK(container.Instructions()[14].Opcode == 47u); // LOG
		CHECK(container.Instructions()[15].TokenOffset == 50u);
		CHECK(container.Instructions()[15].Opcode == 56u); // MUL
		CHECK(container.Instructions()[15].TokenLength == 10u);
		CHECK(container.Instructions()[16].TokenOffset == 60u);
		CHECK(container.Instructions()[16].Opcode == 25u); // EXP
	}

	// ---- Synthetic valid-container tests ----

	void ValidMinimalContainerTests()
	{
		const DxbcInspector inspector;

		{
			// Empty token stream: SHEX with version + length only.
			auto bytes = BuildContainer({ MakeShexChunk({}) });
			auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			CHECK(result.has_value());
			if (result.has_value()) {
				CHECK(result->Chunks().size() == 1u);
				CHECK(result->Instructions().empty());
				CHECK(!result->TargetRegion().has_value());
			}
		}
		{
			// dcl_temps 3; ret
			auto bytes = BuildContainer({ MakeShexChunk({ kDclTemps3, 0x00000003u, kRet }) });
			auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			CHECK(result.has_value());
			if (result.has_value()) {
				CHECK(result->Instructions().size() == 2u);
				CHECK(result->Instructions()[0].Opcode == 104u); // dcl_temps
				CHECK(result->Instructions()[1].Opcode == 62u);  // ret
				CHECK(!result->TargetRegion().has_value());
			}
		}
		{
			// Non-target chunk ordering preserved (ISGN then SHEX).
			auto bytes = BuildContainer({ MakeIsgnChunk(), MakeShexChunk({ kRet }) });
			auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			CHECK(result.has_value());
			if (result.has_value()) {
				CHECK(result->Chunks().size() == 2u);
				CHECK(result->Chunks()[0].FourCC == 0x4E475349u);
				CHECK(result->Chunks()[1].FourCC == 0x58454853u);
			}
		}
	}

	// ---- Malformed-container tests (descriptive offset-bearing errors) ----

	void MalformedContainerTests()
	{
		{
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			bytes[3] = std::byte{ 'D' }; // "DXBD"
			EXPECT_ERROR(bytes, DxbcErrorCode::BadMagic, 0u);
		}
		{
			Bytes bytes;
			for (int i = 0; i < 20; ++i) {
				bytes.push_back(std::byte{ 0 });
			}
			bytes[0] = std::byte{ 'D' }; bytes[1] = std::byte{ 'X' };
			bytes[2] = std::byte{ 'B' }; bytes[3] = std::byte{ 'C' };
			EXPECT_ERROR(bytes, DxbcErrorCode::HeaderTruncated, 20u);
		}
		{
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			SetU32(bytes, 24, static_cast<std::uint32_t>(bytes.size()) + 4u);
			EXPECT_ERROR(bytes, DxbcErrorCode::DeclaredSizeMismatch, 24u);
		}
		{
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			SetU32(bytes, 28, 0xFFFFFFFFu); // excessive chunk count
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunkTableOutOfRange, 32u);
		}
		{
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			SetU32(bytes, 32, 0x00001000u); // chunk offset beyond EOF
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunkHeaderOutOfRange, 0x1000u);
		}
		{
			// Chunk header points into the chunk-offset table (32..36), not beyond EOF.
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			SetU32(bytes, 32, 34u);
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunkHeaderOutOfRange, 34u);
		}
		{
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			const std::uint32_t chunkOffset = GetU32(bytes, 32);
			SetU32(bytes, chunkOffset + 4, 0x00FFFFFFu); // content size past EOF
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunkOutOfRange, chunkOffset);
		}
		{
			// Two overlapping chunks, hand-placed: chunk B's bytes sit at 56,
			// inside chunk A's content span (40..72).
			Bytes bytes;
			bytes.insert(bytes.end(), { std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ 'B' }, std::byte{ 'C' } });
			for (int i = 0; i < 16; ++i) {
				bytes.push_back(std::byte{ 0 });
			}
			PutU32(bytes, 1u);   // version
			PutU32(bytes, 84u);  // total
			PutU32(bytes, 2u);   // chunk count
			PutU32(bytes, 40u);  // chunk A offset
			PutU32(bytes, 56u);  // chunk B offset (inside A's content)
			for (char c : std::string_view("AAAA")) {
				bytes.push_back(static_cast<std::byte>(c));
			}
			PutU32(bytes, 24u); // A content size: span 40..72
			for (int i = 0; i < 8; ++i) {
				bytes.push_back(std::byte{ 0x11 }); // A content bytes 48..56
			}
			for (char c : std::string_view("BBBB")) {
				bytes.push_back(static_cast<std::byte>(c)); // B fourcc at 56
			}
			PutU32(bytes, 4u); // B content size: span 56..68
			for (int i = 0; i < 4; ++i) {
				bytes.push_back(std::byte{ 0x22 }); // B content 64..68
			}
			for (int i = 0; i < 16; ++i) {
				bytes.push_back(std::byte{ 0x11 }); // A content bytes 68..72
			}
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunksOverlap, 56u);
		}
		{
			// Truncated chunk header: chunk offset points 4 bytes before EOF.
			Bytes bytes;
			bytes.insert(bytes.end(), { std::byte{ 'D' }, std::byte{ 'X' }, std::byte{ 'B' }, std::byte{ 'C' } });
			for (int i = 0; i < 16; ++i) {
				bytes.push_back(std::byte{ 0 });
			}
			PutU32(bytes, 1u); PutU32(bytes, 36u); PutU32(bytes, 1u);
			PutU32(bytes, 32u);
			bytes.resize(36u); // only 4 bytes available at chunk offset 32
			EXPECT_ERROR(bytes, DxbcErrorCode::ChunkHeaderOutOfRange, 32u);
		}
		{
			// Missing shader chunk.
			auto bytes = BuildContainer({ MakeIsgnChunk() });
			EXPECT_ERROR(bytes, DxbcErrorCode::MissingShaderChunk, 0u);
		}
		{
			// Duplicate shader chunks.
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }), MakeShexChunk({ kRet }) });
			const std::uint32_t second = GetU32(bytes, 36);
			EXPECT_ERROR(bytes, DxbcErrorCode::DuplicateShaderChunk, second);
		}
		{
			// Bad shader dword length: content smaller than fixed header.
			Bytes content;
			PutU32(content, 0x50u);
			auto bytes = BuildContainer({ MakeShexChunkWithContent(std::move(content)) });
			const std::uint32_t chunkOffset = GetU32(bytes, 32);
			EXPECT_ERROR(bytes, DxbcErrorCode::ShaderLengthMismatch, chunkOffset + 8u);
		}
		{
			// Bad shader dword length: content not a multiple of 4.
			Bytes content;
			PutU32(content, 0x50u); PutU32(content, 3u);
			content.push_back(std::byte{ 0xAA });
			auto bytes = BuildContainer({ MakeShexChunkWithContent(std::move(content)) });
			const std::uint32_t chunkOffset = GetU32(bytes, 32);
			EXPECT_ERROR(bytes, DxbcErrorCode::ShaderLengthMismatch, chunkOffset + 8u);
		}
		{
			// Bad shader dword length: declared length field too small.
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			const std::uint32_t chunkOffset = GetU32(bytes, 32);
			SetU32(bytes, chunkOffset + 12, 1u); // length dwords < 2
			EXPECT_ERROR(bytes, DxbcErrorCode::ShaderLengthMismatch, chunkOffset + 12u);
		}
		{
			// Bad shader dword length: declared length field does not match content.
			auto bytes = BuildContainer({ MakeShexChunk({ kRet }) });
			const std::uint32_t chunkOffset = GetU32(bytes, 32);
			SetU32(bytes, chunkOffset + 12, 100u);
			EXPECT_ERROR(bytes, DxbcErrorCode::ShaderLengthMismatch, chunkOffset + 12u);
		}
		{
			// Zero instruction length.
			auto bytes = BuildContainer({ MakeShexChunk({ 0x00000000u }) });
			EXPECT_ERROR(bytes, DxbcErrorCode::ZeroInstructionLength, 0u);
		}
		{
			// Overlong instruction length.
			auto bytes = BuildContainer({ MakeShexChunk({ 0x7F000000u | 0x2Fu, kRet }) });
			EXPECT_ERROR(bytes, DxbcErrorCode::InstructionOutOfRange, 0u);
		}
		{
			// Truncated tokens: final instruction declares more tokens than remain.
			auto bytes = BuildContainer({ MakeShexChunk({ kRet, kLogR0V2, 0x001000F2u }) });
			EXPECT_ERROR(bytes, DxbcErrorCode::InstructionOutOfRange, 1u);
		}
		{
			// Unsupported custom-data encoding.
			auto bytes = BuildContainer({ MakeShexChunk({ 0x35000035u, 0x00000000u }) });
			EXPECT_ERROR(bytes, DxbcErrorCode::UnsupportedCustomData, 0u);
		}
	}

	// ---- Wrong-sequence safety: valid containers, no target region ----

	void NonTargetSequenceTests()
	{
		const DxbcInspector inspector;

		auto expectNoRegion = [&](const std::vector<std::uint32_t>& tokens, const char* file, int line) {
			auto bytes = BuildContainer({ MakeShexChunk(tokens) });
			auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			Check(result.has_value(), "valid synthetic container rejected", file, line);
			if (result.has_value()) {
				Check(!result->TargetRegion().has_value(), "wrong sequence labeled as target region", file, line);
			}
		};
#define EXPECT_NO_REGION(tokens) expectNoRegion((tokens), __FILE__, __LINE__)

		{
			// Correct region, must be detected.
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u };
			tokens.push_back(kMulR0Gamma);
			{
				const std::vector<std::uint32_t> gamma = GammaImmediateTokens();
				tokens.insert(tokens.end(), gamma.begin(), gamma.end());
			}
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			auto bytes = BuildContainer({ MakeShexChunk(tokens) });
			auto result = inspector.Inspect(ByteView{ bytes.data(), bytes.size() });
			CHECK(result.has_value());
			if (result.has_value()) {
				CHECK(result->TargetRegion().has_value());
				if (result->TargetRegion().has_value()) {
					CHECK(result->TargetRegion()->TokenOffset == 0u);
					CHECK(result->TargetRegion()->TokenLength == 5u);
					CHECK(result->TargetRegion()->SourceRegister == 2u);
				}
			}
		}
		{
			// Same shape but different input register (v1 instead of v2).
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000001u };
			tokens.push_back(kMulR0Gamma);
			{
				const std::vector<std::uint32_t> gamma = GammaImmediateTokens();
				tokens.insert(tokens.end(), gamma.begin(), gamma.end());
			}
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			EXPECT_NO_REGION(tokens);
		}
		{
			// Same shape but three-channel mask on the log source.
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000002u };
			tokens.push_back(kMulR0Gamma);
			{
				const std::vector<std::uint32_t> gamma = GammaImmediateTokens();
				tokens.insert(tokens.end(), gamma.begin(), gamma.end());
			}
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			EXPECT_NO_REGION(tokens);
		}
		{
			// Wrong multiplier immediate (2.0 instead of 2.2).
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u, kMulR0Gamma };
			tokens.insert(tokens.end(), {
				0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u, 0x00004002u,
				0x40000000u, 0x40000000u, 0x40000000u, 0x40000000u,
				kExpR0 });
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			EXPECT_NO_REGION(tokens);
		}
		{
			// LOG not followed by MUL/EXP.
			const std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u, kRet };
			EXPECT_NO_REGION(tokens);
		}
		{
			// Correct operands but wrong order (exp before mul).
			std::vector<std::uint32_t> tokens = { kLogR0V2, 0x001000F2u, 0x00000000u, 0x00101E46u, 0x00000002u };
			tokens.push_back(kExpR0);
			tokens.insert(tokens.end(), { 0x001000F2u, 0x00000000u, 0x00100E46u, 0x00000000u });
			tokens.push_back(kMulR0Gamma);
			{
				const std::vector<std::uint32_t> gamma = GammaImmediateTokens();
				tokens.insert(tokens.end(), gamma.begin(), gamma.end());
			}
			EXPECT_NO_REGION(tokens);
		}
	}
}

auto main() -> int
{
	OriginalOracleTests();
	ValidMinimalContainerTests();
	MalformedContainerTests();
	NonTargetSequenceTests();

	if (g_failures != 0) {
		std::cerr << "Core.DxbcInspector: " << g_failures << " check(s) failed.\n";
		return EXIT_FAILURE;
	}

	std::cout << "Core.DxbcInspector: all checks passed.\n";
	return EXIT_SUCCESS;
}
