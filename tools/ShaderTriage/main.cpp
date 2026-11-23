#include "Core/ShaderTriage.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	struct Entry
	{
		std::string Hash;
		std::uintmax_t Size = 0;
		NaNBuster::ShaderTriageResult Analysis;
		std::vector<std::string> Occurrences;
	};

	auto ReadText(const std::filesystem::path& path) -> std::string
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream) {
			throw std::runtime_error("failed to open " + path.string());
		}
		return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}

	auto ReadU32(const std::array<unsigned char, 20>& bytes, std::size_t offset) -> std::uint32_t
	{
		return static_cast<std::uint32_t>(bytes[offset]) |
			(static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
			(static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
			(static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
	}

	auto EmbeddedHash(const std::filesystem::path& path) -> std::string
	{
		std::ifstream stream(path, std::ios::binary);
		std::array<unsigned char, 20> bytes{};
		if (!stream || !stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
			throw std::runtime_error("failed to read DXBC header " + path.string());
		}
		if (!(bytes[0] == 'D' && bytes[1] == 'X' && bytes[2] == 'B' && bytes[3] == 'C')) {
			throw std::runtime_error("bad DXBC magic " + path.string());
		}
		std::ostringstream result;
		result << std::hex << std::setfill('0');
		for (std::size_t offset = 4; offset < 20; offset += 4) {
			if (offset > 4) {
				result << '-';
			}
			result << std::setw(8) << ReadU32(bytes, offset);
		}
		return result.str();
	}

	auto DisassemblyHash(std::string_view text) -> std::string
	{
		constexpr std::string_view marker = "Shader hash ";
		const std::size_t begin = text.find(marker);
		if (begin == std::string_view::npos) {
			return {};
		}
		const std::size_t value = begin + marker.size();
		const std::size_t end = text.find_first_of(" \t\r\n", value);
		return std::string(text.substr(value, end - value));
	}

	auto JsonEscape(std::string_view value) -> std::string
	{
		std::string result;
		for (const char character : value) {
			switch (character) {
			case '\\': result += "\\\\"; break;
			case '"': result += "\\\""; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '\t': result += "\\t"; break;
			default: result += character; break;
			}
		}
		return result;
	}

	auto JsonStringArray(const std::vector<std::string>& values, int indent) -> std::string
	{
		std::ostringstream output;
		output << "[";
		if (!values.empty()) {
			output << '\n';
			for (std::size_t i = 0; i < values.size(); ++i) {
				output << std::string(static_cast<std::size_t>(indent + 2), ' ') << '"' << JsonEscape(values[i]) << '"';
				if (i + 1 < values.size()) {
					output << ',';
				}
				output << '\n';
			}
			output << std::string(static_cast<std::size_t>(indent), ' ');
		}
		output << ']';
		return output.str();
	}

	void WriteReports(const std::filesystem::path& outputDirectory, std::vector<Entry> entries, const std::vector<std::string>& errors)
	{
		std::sort(entries.begin(), entries.end(), [](const Entry& left, const Entry& right) {
			return left.Size != right.Size ? left.Size < right.Size : left.Hash < right.Hash;
		});
		std::map<std::string, std::size_t> families;
		for (const auto& entry : entries) {
			++families[entry.Analysis.FlowSignature];
		}
		std::filesystem::create_directories(outputDirectory);
		{
			std::ofstream json(outputDirectory / "triage-results.json", std::ios::trunc);
			json << "{\n  \"unique_identities\": " << entries.size() << ",\n";
			json << "  \"family_count\": " << families.size() << ",\n";
			json << "  \"errors\": " << JsonStringArray(errors, 2) << ",\n  \"entries\": [\n";
			for (std::size_t i = 0; i < entries.size(); ++i) {
				const auto& entry = entries[i];
				json << "    {\n      \"hash\": \"" << entry.Hash << "\",\n";
				json << "      \"byte_size\": " << entry.Size << ",\n";
				json << "      \"verdict\": \"" << NaNBuster::ToString(entry.Analysis.Verdict) << "\",\n";
				json << "      \"power_family\": \"" << NaNBuster::ToString(entry.Analysis.PowerFamily) << "\",\n";
				json << "      \"requires_manual_review\": " << (entry.Analysis.RequiresManualReview ? "true" : "false") << ",\n";
				json << "      \"color_register\": \"" << entry.Analysis.ColorRegister << "\",\n";
				json << "      \"flow_signature\": \"" << JsonEscape(entry.Analysis.FlowSignature) << "\",\n";
				json << "      \"occurrences\": " << JsonStringArray(entry.Occurrences, 6) << ",\n";
				json << "      \"evidence\": " << JsonStringArray(entry.Analysis.Evidence, 6) << "\n    }";
				if (i + 1 < entries.size()) {
					json << ',';
				}
				json << '\n';
			}
			json << "  ]\n}\n";
		}
		{
			std::ofstream markdown(outputDirectory / "triage-results.md", std::ios::trunc);
			markdown << "# Shader Triage Results\n\n";
			markdown << "Unique identities: **" << entries.size() << "**  \n";
			markdown << "Flow families: **" << families.size() << "**  \n";
			markdown << "Errors: **" << errors.size() << "**\n\n";
			markdown << "| Bytes | DXBC hash | Verdict | Power | Manual review | Occurrences |\n";
			markdown << "|---:|---|---|---|---|---:|\n";
			for (const auto& entry : entries) {
				markdown << "| " << entry.Size << " | `" << entry.Hash << "` | `"
					<< NaNBuster::ToString(entry.Analysis.Verdict) << "` | `"
					<< NaNBuster::ToString(entry.Analysis.PowerFamily) << "` | "
					<< (entry.Analysis.RequiresManualReview ? "yes" : "no") << " | "
					<< entry.Occurrences.size() << " |\n";
			}
			markdown << "\n## Flow families\n\n";
			for (const auto& [signature, count] : families) {
				markdown << "- " << count << " × `" << signature << "`\n";
			}
			if (!errors.empty()) {
				markdown << "\n## Errors\n\n";
				for (const auto& error : errors) {
					markdown << "- " << error << "\n";
				}
			}
		}
	}
}

auto main(int argc, char* argv[]) -> int
{
	if (argc != 3) {
		std::cerr << "usage: ShaderTriage <candidate-root> <output-directory>\n";
		return 2;
	}
	try {
		const std::filesystem::path inputRoot = argv[1];
		const std::filesystem::path outputDirectory = argv[2];
		std::map<std::string, Entry> entries;
		std::vector<std::string> errors;
		for (const auto& item : std::filesystem::recursive_directory_iterator(inputRoot)) {
			if (!item.is_regular_file() || item.path().extension() != ".asm") {
				continue;
			}
			try {
				const std::filesystem::path dxbcPath = std::filesystem::path(item.path()).replace_extension(".dxbc");
				if (!std::filesystem::is_regular_file(dxbcPath)) {
					throw std::runtime_error("missing paired DXBC");
				}
				const std::string text = ReadText(item.path());
				const std::string hash = DisassemblyHash(text);
				if (hash.empty()) {
					throw std::runtime_error("missing disassembly hash");
				}
				const std::string binaryHash = EmbeddedHash(dxbcPath);
				if (hash != binaryHash) {
					throw std::runtime_error("ASM/DXBC hash mismatch: " + hash + " != " + binaryHash);
				}
				const std::uintmax_t size = std::filesystem::file_size(dxbcPath);
				const std::string key = hash + ":" + std::to_string(size);
				auto analysis = NaNBuster::AnalyzeShaderDisassembly(text);
				const std::string occurrence = item.path().parent_path().filename().string() + "/" + item.path().filename().string();
				auto [found, inserted] = entries.emplace(key, Entry{ hash, size, analysis, { occurrence } });
				if (!inserted) {
					found->second.Occurrences.push_back(occurrence);
					if (found->second.Analysis.FlowSignature != analysis.FlowSignature) {
						throw std::runtime_error("duplicate identity has inconsistent analysis");
					}
				}
			} catch (const std::exception& error) {
				errors.push_back(item.path().string() + ": " + error.what());
			}
		}
		WriteReports(outputDirectory, [&] {
			std::vector<Entry> values;
			for (auto& [key, entry] : entries) {
				values.push_back(std::move(entry));
			}
			return values;
		}(), errors);
		std::cout << "ShaderTriage: analyzed " << entries.size() << " unique identities; errors=" << errors.size()
			<< "; output='" << outputDirectory.string() << "'\n";
		return errors.empty() ? 0 : 3;
	} catch (const std::exception& error) {
		std::cerr << "ShaderTriage: error: " << error.what() << '\n';
		return 4;
	}
}
