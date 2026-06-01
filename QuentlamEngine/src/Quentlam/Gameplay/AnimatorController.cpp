#include "qlpch.h"
#include "AnimatorController.h"
#include <filesystem>

namespace Quentlam
{

AnimatorState* AnimatorController::CreateState(const std::string& name)
{
	if (HasState(name))
		return GetState(name);

	AnimatorState state;
	state.Name = name;
	m_States.push_back(state);
	return &m_States.back();
}

AnimatorState* AnimatorController::GetState(const std::string& name)
{
	for (auto& state : m_States)
	{
		if (state.Name == name)
			return &state;
	}
	return nullptr;
}

bool AnimatorController::HasState(const std::string& name) const
{
	for (const auto& state : m_States)
	{
		if (state.Name == name)
			return true;
	}
	return false;
}

void AnimatorController::RemoveState(const std::string& name)
{
	m_States.erase(
		std::remove_if(m_States.begin(), m_States.end(),
			[&name](const AnimatorState& s) { return s.Name == name; }),
		m_States.end());

	m_Transitions.erase(
		std::remove_if(m_Transitions.begin(), m_Transitions.end(),
			[this, &name](const AnimatorTransition& t) {
				return t.SourceState == name || t.DestinationState == name;
			}),
		m_Transitions.end());
}

AnimatorTransition* AnimatorController::CreateTransition(const std::string& from, const std::string& to)
{
	if (!HasState(from) || !HasState(to))
		return nullptr;

	for (auto& t : m_Transitions)
	{
		if (t.SourceState == from && t.DestinationState == to)
			return &t;
	}

	AnimatorTransition trans;
	trans.SourceState = from;
	trans.DestinationState = to;
	trans.Duration = 0.0f;
	trans.HasExitTime = true;
	trans.ExitTime = 0.5f;
	m_Transitions.push_back(trans);

	for (auto& s : m_States)
	{
		if (s.Name == from)
			s.Transitions.push_back(to);
	}

	return &m_Transitions.back();
}

AnimatorTransition* AnimatorController::GetTransition(const std::string& from, const std::string& to)
{
	for (auto& t : m_Transitions)
	{
		if (t.SourceState == from && t.DestinationState == to)
			return &t;
	}
	return nullptr;
}

void AnimatorController::RemoveTransition(const std::string& from, const std::string& to)
{
	m_Transitions.erase(
		std::remove_if(m_Transitions.begin(), m_Transitions.end(),
			[&from, &to](const AnimatorTransition& t) {
				return t.SourceState == from && t.DestinationState == to;
			}),
		m_Transitions.end());

	for (auto& s : m_States)
	{
		if (s.Name == from)
		{
			s.Transitions.erase(
				std::remove(s.Transitions.begin(), s.Transitions.end(), to),
				s.Transitions.end());
		}
	}
}

std::vector<AnimatorTransition*> AnimatorController::GetTransitionsFrom(const std::string& stateName)
{
	std::vector<AnimatorTransition*> result;
	for (auto& t : m_Transitions)
	{
		if (t.SourceState == stateName)
			result.push_back(&t);
	}
	return result;
}

void AnimatorController::AddParameter(const AnimatorControllerParameter& param)
{
	if (HasParameter(param.Name))
		return;
	m_Parameters.push_back(param);
}

void AnimatorController::RemoveParameter(const std::string& name)
{
	m_Parameters.erase(
		std::remove_if(m_Parameters.begin(), m_Parameters.end(),
			[&name](const AnimatorControllerParameter& p) { return p.Name == name; }),
		m_Parameters.end());
}

AnimatorControllerParameter* AnimatorController::GetParameter(const std::string& name)
{
	for (auto& p : m_Parameters)
	{
		if (p.Name == name)
			return &p;
	}
	return nullptr;
}

bool AnimatorController::HasParameter(const std::string& name) const
{
	for (const auto& p : m_Parameters)
	{
		if (p.Name == name)
			return true;
	}
	return false;
}

bool AnimatorController::SerializeToFile(const std::string& filepath) const
{
	std::string json = SerializeToString();
	FILE* f = fopen(filepath.c_str(), "w");
	if (!f) return false;
	fwrite(json.c_str(), 1, json.size(), f);
	fclose(f);
	return true;
}

bool AnimatorController::DeserializeFromFile(const std::string& filepath)
{
	FILE* f = fopen(filepath.c_str(), "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::string json(len + 1, '\0');
	fread(json.data(), 1, len, f);
	fclose(f);
	json.resize(len);
	return DeserializeFromString(json);
}

std::string AnimatorController::SerializeToString() const
{
	std::string json;
	json.reserve(4096);
	json += "{\n";
	json += "  \"name\": \"" + m_Name + "\",\n";
	json += "  \"entry_state\": \"" + m_EntryState + "\",\n";

	json += "  \"parameters\": [";
	for (size_t i = 0; i < m_Parameters.size(); ++i)
	{
		const auto& p = m_Parameters[i];
		if (i > 0) json += ",";
		json += "\n    { \"name\": \"" + p.Name + "\", \"type\": ";
		switch (p.Type)
		{
		case EAnimatorControllerParameterType::Float: json += "\"Float\""; break;
		case EAnimatorControllerParameterType::Int: json += "\"Int\""; break;
		case EAnimatorControllerParameterType::Bool: json += "\"Bool\""; break;
		case EAnimatorControllerParameterType::Trigger: json += "\"Trigger\""; break;
		}
		json += " }";
	}
	json += "\n  ],\n";

	json += "  \"states\": [";
	for (size_t i = 0; i < m_States.size(); ++i)
	{
		const auto& s = m_States[i];
		if (i > 0) json += ",";
		json += "\n    { \"name\": \"" + s.Name + "\", ";
		json += "\"motion\": \"" + s.MotionPath + "\", ";
		json += "\"speed\": " + std::to_string(s.Speed) + ", ";
		json += "\"pos_x\": " + std::to_string(s.Position.x) + ", ";
		json += "\"pos_y\": " + std::to_string(s.Position.y) + " }";
	}
	json += "\n  ],\n";

	json += "  \"transitions\": [";
	for (size_t i = 0; i < m_Transitions.size(); ++i)
	{
		const auto& t = m_Transitions[i];
		if (i > 0) json += ",";
		json += "\n    { \"from\": \"" + t.SourceState + "\", \"to\": \"" + t.DestinationState + "\", ";
		json += "\"duration\": " + std::to_string(t.Duration) + ", ";
		json += "\"exit_time\": " + std::to_string(t.ExitTime) + ", ";
		json += "\"has_exit_time\": " + std::string(t.HasExitTime ? "true" : "false") + " }";
	}
	json += "\n  ]\n";
	json += "}\n";
	return json;
}

bool AnimatorController::DeserializeFromString(const std::string& str)
{
	m_States.clear();
	m_Transitions.clear();
	m_Parameters.clear();
	m_EntryState.clear();

	size_t p = 0;

	auto SkipWhite = [&str, &p]() {
		while (p < str.size() && std::isspace((unsigned char)str[p])) p++;
	};

	auto ReadString = [&str, &p, &SkipWhite]() -> std::string {
		SkipWhite();
		if (p < str.size() && (str[p] == '"' || str[p] == '\'')) {
			char quote = str[p++];
			size_t start = p;
			while (p < str.size() && str[p] != quote) {
				if (str[p] == '\\') p++;
				p++;
			}
			std::string result = str.substr(start, p - start);
			if (p < str.size()) p++;
			return result;
		}
		return "";
	};

	auto ReadNumber = [&str, &p, &SkipWhite]() -> double {
		SkipWhite();
		size_t start = p;
		if (p < str.size() && (str[p] == '-' || str[p] == '+')) p++;
		while (p < str.size() && (std::isdigit((unsigned char)str[p]) || str[p] == '.' || str[p] == 'e' || str[p] == 'E' || str[p] == '-' || str[p] == '+')) p++;
		return std::stod(str.substr(start, p - start));
	};

	auto ReadBool = [&str, &p, &SkipWhite]() -> bool {
		SkipWhite();
		size_t start = p;
		while (p < str.size() && std::isalpha((unsigned char)str[p])) p++;
		std::string val = str.substr(start, p - start);
		return val == "true" || val == "True" || val == "TRUE";
	};

	auto Expect = [&str, &p](char c) -> bool {
		while (p < str.size() && std::isspace((unsigned char)str[p])) p++;
		if (p < str.size() && str[p] == c) { p++; return true; }
		return false;
	};

	auto ReadKey = [&ReadString]() -> std::string {
		return ReadString();
	};

	SkipWhite();
	if (p < str.size() && str[p] == '{') p++;
	else return false;

	while (true)
	{
		SkipWhite();
		if (p >= str.size()) break;
		if (str[p] == '}') { p++; break; }
		if (str[p] == ',') { p++; continue; }

		std::string key = ReadKey();
		Expect(':');

		if (key == "name") m_Name = ReadString();
		else if (key == "entry_state") m_EntryState = ReadString();
		else if (key == "parameters")
		{
			Expect('[');
			while (true)
			{
				SkipWhite();
				if (p < str.size() && str[p] == ']') { p++; break; }
				if (str[p] == ',') { p++; continue; }
				Expect('{');
				AnimatorControllerParameter param;
				for (int i = 0; i < 2; i++)
				{
					SkipWhite();
					std::string pk = ReadKey();
					Expect(':');
					if (pk == "name") param.Name = ReadString();
					else if (pk == "type")
					{
						std::string t = ReadString();
						if (t == "Float") param.Type = EAnimatorControllerParameterType::Float;
						else if (t == "Int") param.Type = EAnimatorControllerParameterType::Int;
						else if (t == "Bool") param.Type = EAnimatorControllerParameterType::Bool;
						else if (t == "Trigger") param.Type = EAnimatorControllerParameterType::Trigger;
					}
					SkipWhite();
					if (str[p] == ',') p++;
				}
				Expect('}');
				m_Parameters.push_back(param);
			}
		}
		else if (key == "states")
		{
			Expect('[');
			while (true)
			{
				SkipWhite();
				if (p < str.size() && str[p] == ']') { p++; break; }
				if (str[p] == ',') { p++; continue; }
				Expect('{');
				AnimatorState state;
				for (int i = 0; i < 6; i++)
				{
					SkipWhite();
					std::string sk = ReadKey();
					Expect(':');
					if (sk == "name") state.Name = ReadString();
					else if (sk == "motion") state.MotionPath = ReadString();
					else if (sk == "speed") state.Speed = static_cast<float>(ReadNumber());
					else if (sk == "pos_x") state.Position.x = static_cast<float>(ReadNumber());
					else if (sk == "pos_y") state.Position.y = static_cast<float>(ReadNumber());
					SkipWhite();
					if (str[p] == ',') p++;
				}
				Expect('}');
				m_States.push_back(state);
			}
		}
		else if (key == "transitions")
		{
			Expect('[');
			while (true)
			{
				SkipWhite();
				if (p < str.size() && str[p] == ']') { p++; break; }
				if (str[p] == ',') { p++; continue; }
				Expect('{');
				AnimatorTransition trans;
				for (int i = 0; i < 5; i++)
				{
					SkipWhite();
					std::string tk = ReadKey();
					Expect(':');
					if (tk == "from") trans.SourceState = ReadString();
					else if (tk == "to") trans.DestinationState = ReadString();
					else if (tk == "duration") trans.Duration = static_cast<float>(ReadNumber());
					else if (tk == "exit_time") trans.ExitTime = static_cast<float>(ReadNumber());
					else if (tk == "has_exit_time") trans.HasExitTime = ReadBool();
					SkipWhite();
					if (str[p] == ',') p++;
				}
				Expect('}');
				m_Transitions.push_back(trans);
			}
		}
		else
		{
			SkipWhite();
			while (p < str.size() && str[p] != ',' && str[p] != '}') p++;
		}
	}

	return true;
}

}
