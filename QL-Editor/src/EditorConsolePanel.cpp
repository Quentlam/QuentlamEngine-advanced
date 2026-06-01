#include "EditorConsolePanel.h"
#include "Quentlam/DebugTools/DebugToolsModule.h"
#include "imgui/imgui.h"
#include <vector>
#include <string>
#include <cctype>

namespace Quentlam
{

static const char* LevelPrefix(const std::string& msg)
{
	if (msg.find("[ERROR]") != std::string::npos) return "Error";
	if (msg.find("[WARN]") != std::string::npos)  return "Warn";
	if (msg.find("[TRACE]") != std::string::npos) return "Trace";
	if (msg.find("[INFO]") != std::string::npos)  return "Info";
	if (msg.find("[DEBUG]") != std::string::npos) return "Debug";
	return "Info";
}

void EditorConsolePanel::Clear()
{
	DebugConsole::Get().ClearOutput();
	m_InputBuffer[0] = '\0';
	m_FilterBuffer[0] = '\0';
	m_FilteredHistory.clear();
}

void EditorConsolePanel::OnImGuiRender()
{
	if (!m_IsOpen) return;

	ImGui::SetNextWindowSize(ImVec2(700.0f, 300.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console", &m_IsOpen,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	DrawToolbar();
	DrawOutput();
	DrawInput();

	ImGui::End();
}

void EditorConsolePanel::DrawToolbar()
{
	float btnHeight = ImGui::GetFrameHeight();

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

	if (ImGui::Button("Clear", ImVec2(60.0f, btnHeight)))
		Clear();

	ImGui::SameLine();

	if (ImGui::Button("Copy", ImVec2(55.0f, btnHeight)))
	{
		std::string all;
		const auto& history = DebugConsole::Get().GetOutputHistory();
		for (const auto& line : history)
			all += line + "\n";
		ImGui::SetClipboardText(all.c_str());
	}

	ImGui::SameLine();
	ImGui::Text("Filter:");
	ImGui::SameLine();
	ImGui::PushItemWidth(200.0f);
	ImGui::InputText("##ConsoleFilter", m_FilterBuffer, sizeof(m_FilterBuffer));
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::Checkbox("AutoScroll", &m_AutoScroll);

	ImGui::PopStyleVar();

	ImGui::Separator();
}

void EditorConsolePanel::DrawOutput()
{
	ImGui::BeginChild("ConsoleOutput", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4.0f));

	const auto& output = DebugConsole::Get().GetOutputHistory();
	if (static_cast<int>(output.size()) != m_LastOutputSize)
	{
		m_FilteredHistory.clear();
		for (const auto& line : output)
		{
			if (m_FilterBuffer[0] == '\0')
			{
				m_FilteredHistory.push_back(line);
			}
			else
			{
				std::string lowerLine = line;
				std::string lowerFilter = m_FilterBuffer;
				for (auto& c : lowerLine) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				for (auto& c : lowerFilter) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				if (lowerLine.find(lowerFilter) != std::string::npos)
					m_FilteredHistory.push_back(line);
			}
		}
		m_LastOutputSize = static_cast<int>(output.size());
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f));

	for (const auto& line : m_FilteredHistory)
	{
		ImVec4 textColor(0.9f, 0.9f, 0.9f, 1.0f);
		std::string prefix = LevelPrefix(line);
		if (prefix == "Error")
			textColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
		else if (prefix == "Warn")
			textColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
		else if (prefix == "Trace")
			textColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
		else if (prefix == "Debug")
			textColor = ImVec4(0.5f, 0.8f, 0.5f, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
		ImGui::TextWrapped("%s", line.c_str());
		ImGui::PopStyleColor();
	}

	if (m_AutoScroll && !output.empty())
		ImGui::SetScrollHereY(1.0f);

	ImGui::PopStyleVar();
	ImGui::EndChild();
}

void EditorConsolePanel::DrawInput()
{
	ImGui::Separator();

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("> ");
	ImGui::SameLine();

	ImGui::PushItemWidth(-1.0f);

	if (ImGui::InputText("##ConsoleInput", m_InputBuffer, sizeof(m_InputBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
		[](ImGuiInputTextCallbackData* data) -> int
		{
			const auto& cmdHistory = DebugConsole::Get().GetCommandHistory();
			static int historyIdx = -1;

			if (data->EventKey == ImGuiKey_UpArrow)
			{
				if (cmdHistory.empty()) return 0;
				historyIdx = historyIdx < 0
					? static_cast<int>(cmdHistory.size()) - 1
					: (historyIdx > 0 ? historyIdx - 1 : historyIdx);
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, cmdHistory[historyIdx].c_str());
			}
			else if (data->EventKey == ImGuiKey_DownArrow)
			{
				if (cmdHistory.empty()) return 0;
				historyIdx = historyIdx < static_cast<int>(cmdHistory.size()) - 1
					? historyIdx + 1
					: static_cast<int>(cmdHistory.size());
				data->DeleteChars(0, data->BufTextLen);
				if (historyIdx < static_cast<int>(cmdHistory.size()))
					data->InsertChars(0, cmdHistory[historyIdx].c_str());
			}
			return 0;
		},
		this))
	{
		std::string cmd = m_InputBuffer;
		if (!cmd.empty())
		{
			DebugConsole::Get().Execute(cmd);
			m_InputBuffer[0] = '\0';
		}
	}

	ImGui::PopItemWidth();
	ImGui::PopStyleVar();
}

} // namespace Quentlam
