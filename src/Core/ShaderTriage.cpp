#include "Core/ShaderTriage.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace NaNBuster
{
	namespace
	{
		using Channels = std::array<bool, 4>;
		using FlowState = std::map<std::string, Channels>;

		struct RegisterRef
		{
			std::string Name;
			std::array<int, 4> Components = { 0, 1, 2, 3 };
			std::size_t Count = 4;
		};

		struct Instruction
		{
			std::string Text;
			std::string Opcode;
			std::vector<std::string> Operands;
		};

		auto Trim(std::string_view value) -> std::string_view
		{
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
				value.remove_prefix(1);
			}
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
				value.remove_suffix(1);
			}
			return value;
		}

		auto StartsWith(std::string_view value, std::string_view prefix) -> bool
		{
			return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
		}

		auto SplitOperands(std::string_view text) -> std::vector<std::string>
		{
			std::vector<std::string> operands;
			int depth = 0;
			std::size_t begin = 0;
			for (std::size_t i = 0; i < text.size(); ++i) {
				if (text[i] == '(') {
					++depth;
				} else if (text[i] == ')') {
					--depth;
				} else if (text[i] == ',' && depth == 0) {
					operands.emplace_back(Trim(text.substr(begin, i - begin)));
					begin = i + 1;
				}
			}
			if (begin < text.size()) {
				operands.emplace_back(Trim(text.substr(begin)));
			}
			return operands;
		}

		auto IsUnnumberedOpcode(std::string_view opcode) -> bool
		{
			static const std::set<std::string_view> opcodes = {
				"add", "add_sat", "and", "break", "breakc", "continue", "deriv_rtx", "deriv_rty",
				"discard_nz", "discard_z", "div", "dp2", "dp3", "dp4", "else", "endif", "endloop",
				"endswitch", "eq", "exp", "frc", "ftoi", "ftou", "ge", "if_nz", "if_z", "ilog",
				"ine", "itof", "ld", "ld_indexable", "log", "loop", "lt", "mad", "max", "min",
				"mov", "mul", "mul_sat", "ne", "or", "rcp", "resinfo", "ret", "round_ne", "round_ni",
				"round_pi", "round_z", "rsq", "sample", "sample_indexable", "sincos", "sqrt", "sub",
				"switch", "udiv", "umad", "umax", "umin", "umul", "utof", "xor",
			};
			if (opcodes.contains(opcode)) {
				return true;
			}
			return StartsWith(opcode, "sample_") || StartsWith(opcode, "ld_");
		}

		auto ParseInstruction(std::string_view line) -> std::optional<Instruction>
		{
			line = Trim(line);
			if (line.empty()) {
				return std::nullopt;
			}
			std::size_t cursor = 0;
			while (cursor < line.size() && std::isdigit(static_cast<unsigned char>(line[cursor])) != 0) {
				++cursor;
			}
			bool numbered = cursor > 0 && cursor < line.size() && line[cursor] == ':';
			if (numbered) {
				line = Trim(line.substr(cursor + 1));
			}
			const std::size_t split = line.find_first_of(" \t");
			const std::string_view opcode = Trim(line.substr(0, split));
			if (!numbered && !IsUnnumberedOpcode(opcode)) {
				return std::nullopt;
			}
			Instruction instruction;
			instruction.Text = std::string(line);
			instruction.Opcode = std::string(opcode);
			if (split != std::string_view::npos) {
				instruction.Operands = SplitOperands(Trim(line.substr(split + 1)));
			}
			return instruction;
		}

		auto ParseInstructions(std::string_view text) -> std::vector<Instruction>
		{
			std::vector<Instruction> instructions;
			std::size_t begin = 0;
			while (begin <= text.size()) {
				const std::size_t end = text.find('\n', begin);
				auto instruction = ParseInstruction(text.substr(begin, end == std::string_view::npos ? end : end - begin));
				if (instruction) {
					instructions.push_back(std::move(*instruction));
				}
				if (end == std::string_view::npos) {
					break;
				}
				begin = end + 1;
			}
			return instructions;
		}

		auto ParseRegister(std::string_view operand, bool allowInput = false) -> std::optional<RegisterRef>
		{
			operand = Trim(operand);
			if (!operand.empty() && (operand.front() == '-' || operand.front() == '+')) {
				operand.remove_prefix(1);
			}
			const std::size_t end = operand.find_first_of(" \t");
			operand = operand.substr(0, end);
			if (operand.size() < 2 || !((operand[0] == 'r' || operand[0] == 'o') ||
				(allowInput && operand[0] == 'v'))) {
				return std::nullopt;
			}
			std::size_t cursor = 1;
			while (cursor < operand.size() && std::isdigit(static_cast<unsigned char>(operand[cursor])) != 0) {
				++cursor;
			}
			if (cursor == 1) {
				return std::nullopt;
			}
			RegisterRef result;
			result.Name = std::string(operand.substr(0, cursor));
			if (cursor == operand.size()) {
				return result;
			}
			if (operand[cursor] != '.') {
				return std::nullopt;
			}
			++cursor;
			result.Count = 0;
			for (; cursor < operand.size(); ++cursor) {
				const char component = static_cast<char>(std::tolower(static_cast<unsigned char>(operand[cursor])));
				const std::string_view names = "xyzw";
				const std::size_t index = names.find(component);
				if (index == std::string_view::npos || result.Count >= 4) {
					return std::nullopt;
				}
				result.Components[result.Count++] = static_cast<int>(index);
			}
			return result.Count > 0 ? std::optional<RegisterRef>(result) : std::nullopt;
		}

		auto Any(const Channels& channels) -> bool
		{
			return std::any_of(channels.begin(), channels.end(), [](bool value) { return value; });
		}

		auto Covers(const RegisterRef& destination, const RegisterRef& source) -> bool
		{
			for (std::size_t i = 0; i < source.Count; ++i) {
				const int component = source.Components[i];
				const auto begin = destination.Components.begin();
				const auto end = destination.Components.begin() + static_cast<std::ptrdiff_t>(destination.Count);
				if (std::find(begin, end, component) == end) {
					return false;
				}
			}
			return true;
		}

		auto IsZeroImmediate(std::string_view operand) -> bool
		{
			operand = Trim(operand);
			if (!StartsWith(operand, "l(") || operand.back() != ')') {
				return false;
			}
			operand.remove_prefix(2);
			operand.remove_suffix(1);
			for (const auto& component : SplitOperands(operand)) {
				try {
					if (std::stod(component) != 0.0) {
						return false;
					}
				} catch (...) {
					return false;
				}
			}
			return true;
		}

		auto CountGamma(std::string_view text) -> std::size_t
		{
			std::size_t count = 0;
			std::size_t position = 0;
			while ((position = text.find("2.200000", position)) != std::string_view::npos) {
				++count;
				position += 8;
			}
			return count;
		}

		auto ClassifyPower(const std::vector<Instruction>& instructions, std::size_t logIndex, const RegisterRef& color)
			-> ShaderPowerFamily
		{
			if (logIndex + 2 >= instructions.size() || instructions[logIndex + 1].Opcode != "mul" ||
				instructions[logIndex + 2].Opcode != "exp") {
				return ShaderPowerFamily::Unknown;
			}
			const auto destination = ParseRegister(instructions[logIndex].Operands.at(0));
			if (!destination) {
				return ShaderPowerFamily::Unknown;
			}
			const std::size_t gamma = CountGamma(instructions[logIndex + 1].Text);
			if (destination->Count == 4 && gamma >= 4) {
				return ShaderPowerFamily::Rgba22;
			}
			if (destination->Count == 3 && gamma >= 3) {
				return ShaderPowerFamily::Rgb22;
			}
			if (destination->Count == 1 && color.Count == 1 && color.Components[0] == 3 && gamma >= 1) {
				return ShaderPowerFamily::Alpha22;
			}
			return gamma > 0 ? ShaderPowerFamily::Mixed22 : ShaderPowerFamily::Other;
		}

		auto IsControlFlow(std::string_view opcode) -> bool
		{
			static const std::set<std::string_view> flow = {
				"break", "breakc", "continue", "discard_nz", "discard_z", "else", "endif", "endloop",
				"endswitch", "if_nz", "if_z", "loop", "switch",
			};
			return flow.contains(opcode);
		}

		auto IsNonWriting(std::string_view opcode) -> bool
		{
			return IsControlFlow(opcode) || opcode == "ret" || opcode == "case" || opcode == "default";
		}

		auto IsConversion(std::string_view opcode) -> bool
		{
			static const std::set<std::string_view> conversions = {
				"and", "eq", "ftoi", "ftou", "ge", "ilog", "ine", "itof", "lt", "ne", "or", "udiv", "utof", "xor",
			};
			return conversions.contains(opcode);
		}

		auto IsTextureWrite(std::string_view opcode) -> bool
		{
			return StartsWith(opcode, "sample") || StartsWith(opcode, "ld") || opcode == "resinfo";
		}

		auto IsKnownPropagation(std::string_view opcode) -> bool
		{
			static const std::set<std::string_view> propagation = {
				"add", "deriv_rtx", "deriv_rty", "div", "dp2", "dp3", "dp4", "exp", "frc", "log", "mad",
				"max", "min", "mov", "mul", "rcp", "round_ne", "round_ni", "round_pi", "round_z", "rsq",
				"sincos", "sqrt", "sub", "umad", "umax", "umin", "umul", "add_sat", "mul_sat",
			};
			return propagation.contains(opcode);
		}

		auto JoinSignature(ShaderPowerFamily family, const std::vector<std::string>& operations) -> std::string
		{
			std::string result(ToString(family));
			for (const auto& operation : operations) {
				result += '|';
				result += operation;
			}
			return result;
		}
	}

	auto ToString(ShaderTriageVerdict verdict) -> std::string_view
	{
		switch (verdict) {
		case ShaderTriageVerdict::NotCandidate: return "not_candidate";
		case ShaderTriageVerdict::GuardedBeforeLog: return "guarded_before_log";
		case ShaderTriageVerdict::UnguardedReachesOutput: return "unguarded_reaches_output";
		case ShaderTriageVerdict::UnguardedNoObservedOutput: return "unguarded_no_observed_output";
		case ShaderTriageVerdict::Uncertain: return "uncertain";
		}
		return "unknown";
	}

	auto ToString(ShaderPowerFamily family) -> std::string_view
	{
		switch (family) {
		case ShaderPowerFamily::NotApplicable: return "not_applicable";
		case ShaderPowerFamily::Rgba22: return "rgba_22";
		case ShaderPowerFamily::Rgb22: return "rgb_22";
		case ShaderPowerFamily::Alpha22: return "alpha_22";
		case ShaderPowerFamily::Mixed22: return "mixed_22";
		case ShaderPowerFamily::Other: return "other_power";
		case ShaderPowerFamily::Unknown: return "unknown_power";
		}
		return "unknown_power";
	}

	auto AnalyzeShaderDisassembly(std::string_view disassembly) -> ShaderTriageResult
	{
		const auto instructions = ParseInstructions(disassembly);
		ShaderTriageResult result;
		if (instructions.empty()) {
			return result;
		}

		std::size_t logIndex = 0;
		RegisterRef color;
		RegisterRef logDestination;
		bool guarded = false;
		if (instructions[0].Opcode == "log" && instructions[0].Operands.size() >= 2) {
			const auto destination = ParseRegister(instructions[0].Operands[0]);
			const auto source = ParseRegister(instructions[0].Operands[1], true);
			if (!destination || !source || source->Name.empty() || source->Name[0] != 'v') {
				return result;
			}
			color = *source;
			logDestination = *destination;
		} else if (instructions.size() >= 2 && instructions[0].Opcode == "max" &&
			instructions[0].Operands.size() >= 3 && instructions[1].Opcode == "log" &&
			instructions[1].Operands.size() >= 2 && IsZeroImmediate(instructions[0].Operands[2])) {
			const auto guardDestination = ParseRegister(instructions[0].Operands[0]);
			const auto source = ParseRegister(instructions[0].Operands[1], true);
			const auto logDestinationValue = ParseRegister(instructions[1].Operands[0]);
			const auto logSource = ParseRegister(instructions[1].Operands[1]);
			if (!guardDestination || !source || !logDestinationValue || !logSource || source->Name[0] != 'v' ||
				guardDestination->Name != logSource->Name) {
				return result;
			}
			if (!Covers(*guardDestination, *logSource)) {
				result.Verdict = ShaderTriageVerdict::Uncertain;
				result.RequiresManualReview = true;
				result.ColorRegister = source->Name;
				return result;
			}
			guarded = true;
			logIndex = 1;
			color = *source;
			logDestination = *logDestinationValue;
		} else {
			return result;
		}

		result.ColorRegister = color.Name;
		result.PowerFamily = ClassifyPower(instructions, logIndex, color);
		if (guarded) {
			result.Verdict = ShaderTriageVerdict::GuardedBeforeLog;
			result.FlowSignature = JoinSignature(result.PowerFamily, { "max_zero", "log" });
			result.Evidence = { instructions[0].Text, instructions[1].Text };
			return result;
		}

		FlowState state;
		Channels initial{};
		for (std::size_t i = 0; i < logDestination.Count; ++i) {
			initial[static_cast<std::size_t>(logDestination.Components[i])] = true;
		}
		state[logDestination.Name] = initial;
		std::vector<std::string> operations = { "log" };
		result.Evidence.push_back(instructions[logIndex].Text);
		bool outputDependency = false;

		for (std::size_t i = logIndex + 1; i < instructions.size(); ++i) {
			const Instruction& instruction = instructions[i];
			if (IsControlFlow(instruction.Opcode)) {
				result.RequiresManualReview = true;
			}
			if (IsNonWriting(instruction.Opcode)) {
				continue;
			}
			if (instruction.Operands.empty()) {
				continue;
			}
			const auto destination = ParseRegister(instruction.Operands[0]);
			if (!destination) {
				continue;
			}
			const FlowState previous = state;
			Channels propagated{};
			bool taintedSource = false;
			for (std::size_t operandIndex = 1; operandIndex < instruction.Operands.size(); ++operandIndex) {
				const auto source = ParseRegister(instruction.Operands[operandIndex]);
				if (!source) {
					continue;
				}
				const auto found = previous.find(source->Name);
				if (found == previous.end()) {
					continue;
				}
				for (std::size_t channel = 0; channel < destination->Count; ++channel) {
					const std::size_t sourcePosition = source->Count == 1 ? 0 : std::min(channel, source->Count - 1);
					const int sourceComponent = source->Components[sourcePosition];
					if (found->second[static_cast<std::size_t>(sourceComponent)]) {
						propagated[static_cast<std::size_t>(destination->Components[channel])] = true;
						taintedSource = true;
					}
				}
			}

			const auto oldDestination = previous.find(destination->Name);
			const bool previouslyTainted = oldDestination != previous.end() && Any(oldDestination->second);
			const bool conversion = IsConversion(instruction.Opcode);
			const bool texture = IsTextureWrite(instruction.Opcode);
			const bool knownPropagation = IsKnownPropagation(instruction.Opcode);
			if (taintedSource && (conversion || texture || !knownPropagation)) {
				result.RequiresManualReview = true;
			}
			Channels updated = previous.contains(destination->Name) ? previous.at(destination->Name) : Channels{};
			for (std::size_t channel = 0; channel < destination->Count; ++channel) {
				updated[static_cast<std::size_t>(destination->Components[channel])] =
					(!conversion && !texture) && propagated[static_cast<std::size_t>(destination->Components[channel])];
			}
			state[destination->Name] = updated;
			if (taintedSource || previouslyTainted) {
				result.Evidence.push_back(instruction.Text);
				operations.push_back(instruction.Opcode);
			}
			if (!destination->Name.empty() && destination->Name[0] == 'o' && Any(updated)) {
				outputDependency = true;
			}
		}

		if (outputDependency) {
			result.Verdict = ShaderTriageVerdict::UnguardedReachesOutput;
		} else {
			result.Verdict = result.RequiresManualReview ? ShaderTriageVerdict::Uncertain :
				ShaderTriageVerdict::UnguardedNoObservedOutput;
		}
		result.FlowSignature = JoinSignature(result.PowerFamily, operations);
		return result;
	}
}
