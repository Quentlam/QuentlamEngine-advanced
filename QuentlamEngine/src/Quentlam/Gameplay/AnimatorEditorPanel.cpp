#include "qlpch.h"
#include "AnimatorEditorPanel.h"
#include "AnimatorController.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <Commdlg.h>

namespace Quentlam
{

AnimatorEditorPanel& AnimatorEditorPanel::Get()
{
	static AnimatorEditorPanel instance;
	return instance;
}

void AnimatorEditorPanel::OnImGuiRender()
{
	if (!m_IsOpen)
		return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
	ImGui::Begin("Animator Editor", &m_IsOpen, flags);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Controller"))
			{
				m_Controller = CreateRef<AnimatorController>("NewController");
				m_States.clear();
				m_Transitions.clear();
				m_Parameters.clear();
				m_SelectedState.clear();
			}
			if (ImGui::MenuItem("Load Controller..."))
			{
				char filepath[512] = { 0 };
				OPENFILENAMEA ofn = {};
				ofn.lStructSize = sizeof(ofn);
				ofn.lpstrFilter = "Animator Controller (*.controller)\0*.controller\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = filepath;
				ofn.nMaxFile = 511;
				ofn.lpstrTitle = "Load Animator Controller";
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
				if (GetOpenFileNameA(&ofn))
				{
					m_Controller = CreateRef<AnimatorController>();
					if (m_Controller->DeserializeFromFile(std::string(filepath)))
					{
						m_States.clear();
						m_Transitions.clear();
						m_Parameters.clear();
						for (const auto& s : m_Controller->GetStates())
						{
							AnimatorEditorState es;
							es.Name = s.Name;
							es.ClipPath = s.MotionPath;
							es.Position = s.Position;
							es.Speed = s.Speed;
							m_States.push_back(es);
						}
						for (const auto& t : m_Controller->GetTransitions())
						{
							AnimatorEditorTransition et;
							et.FromState = t.SourceState;
							et.ToState = t.DestinationState;
							et.Duration = t.Duration;
							et.ExitTime = t.ExitTime;
							et.HasExitTime = t.HasExitTime;
							m_Transitions.push_back(et);
						}
						for (const auto& p : m_Controller->GetParameters())
						{
							AnimatorParameter ip;
							ip.Name = p.Name;
							switch (p.Type)
							{
							case EAnimatorControllerParameterType::Float: ip.Type = AnimatorParameter::EType::Float; break;
							case EAnimatorControllerParameterType::Int: ip.Type = AnimatorParameter::EType::Int; break;
							case EAnimatorControllerParameterType::Bool: ip.Type = AnimatorParameter::EType::Bool; break;
							case EAnimatorControllerParameterType::Trigger: ip.Type = AnimatorParameter::EType::Trigger; break;
							}
							m_Parameters.push_back(ip);
						}
						m_SelectedState.clear();
						QL_CORE_INFO("AnimatorController loaded from {0}", filepath);
					}
				}
			}
			if (ImGui::MenuItem("Save Controller..."))
			{
				if (!m_Controller)
					m_Controller = CreateRef<AnimatorController>("NewController");
				char filepath[512] = { 0 };
				OPENFILENAMEA ofn = {};
				ofn.lStructSize = sizeof(ofn);
				ofn.lpstrFilter = "Animator Controller (*.controller)\0*.controller\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = filepath;
				ofn.nMaxFile = 511;
				ofn.lpstrTitle = "Save Animator Controller";
				ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
				if (GetSaveFileNameA(&ofn))
				{
					m_Controller->SetName(std::filesystem::path(filepath).stem().string());
					for (const auto& es : m_States)
					{
						if (!m_Controller->HasState(es.Name))
							m_Controller->CreateState(es.Name);
						auto* s = m_Controller->GetState(es.Name);
						if (s) { s->MotionPath = es.ClipPath; s->Position = es.Position; s->Speed = es.Speed; }
					}
					for (const auto& et : m_Transitions)
					{
						if (!m_Controller->GetTransition(et.FromState, et.ToState))
							m_Controller->CreateTransition(et.FromState, et.ToState);
						auto* t = m_Controller->GetTransition(et.FromState, et.ToState);
						if (t) { t->Duration = et.Duration; t->ExitTime = et.ExitTime; t->HasExitTime = et.HasExitTime; }
					}
					for (const auto& p : m_Parameters)
					{
						if (!m_Controller->HasParameter(p.Name))
						{
							AnimatorControllerParameter cp;
							cp.Name = p.Name;
							switch (p.Type)
							{
							case AnimatorParameter::EType::Float: cp.Type = EAnimatorControllerParameterType::Float; break;
							case AnimatorParameter::EType::Int: cp.Type = EAnimatorControllerParameterType::Int; break;
							case AnimatorParameter::EType::Bool: cp.Type = EAnimatorControllerParameterType::Bool; break;
							case AnimatorParameter::EType::Trigger: cp.Type = EAnimatorControllerParameterType::Trigger; break;
							}
							m_Controller->AddParameter(cp);
						}
					}
					if (m_Controller->SerializeToFile(std::string(filepath)))
						QL_CORE_INFO("AnimatorController saved to {0}", filepath);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::BeginChild("##anim_left_panel", ImVec2(200.0f, 0), false);
	ImGui::Text("Parameters");
	ImGui::Separator();
	RenderParametersPanel();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##anim_right_panel", ImVec2(0, 0), false);
	RenderStateMachineCanvas();
	ImGui::EndChild();

	ImGui::End();
}

void AnimatorEditorPanel::RenderParametersPanel()
{
	if (ImGui::Button("+ Add Parameter", ImVec2(-1, 0)))
	{
		ImGui::OpenPopup("AddParameterPopup");
	}

	if (ImGui::BeginPopup("AddParameterPopup"))
	{
		static char paramName[64] = "NewParam";
		static int paramType = 0;
		ImGui::InputText("Name", paramName, sizeof(paramName));
		ImGui::Combo("Type", &paramType, "Float\0Int\0Bool\0Trigger\0");
		if (ImGui::Button("Add"))
		{
			AnimatorParameter p;
			p.Name = paramName;
			p.Type = static_cast<AnimatorParameter::EType>(paramType);
			AddParameter(p);
			memset(paramName, 0, sizeof(paramName));
			strcpy(paramName, "NewParam");
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::Spacing();
	for (size_t i = 0; i < m_Parameters.size(); ++i)
	{
		auto& p = m_Parameters[i];
		ImGui::PushID(static_cast<int>(i));
		std::string typeStr;
		switch (p.Type)
		{
		case AnimatorParameter::EType::Float: typeStr = "Float"; break;
		case AnimatorParameter::EType::Int: typeStr = "Int"; break;
		case AnimatorParameter::EType::Bool: typeStr = "Bool"; break;
		case AnimatorParameter::EType::Trigger: typeStr = "Trigger"; break;
		}
		if (ImGui::Selectable((p.Name + " (" + typeStr + ")").c_str(), false))
		{
		}
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("ParamContext");
		}
		if (ImGui::BeginPopup("ParamContext"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				m_Parameters.erase(m_Parameters.begin() + i);
				ImGui::EndPopup();
				ImGui::PopID();
				break;
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}
}

void AnimatorEditorPanel::RenderStateMachineCanvas()
{
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	if (canvasSize.x <= 0 || canvasSize.y <= 0)
		return;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImVec2 canvasMin = ImGui::GetCursorScreenPos();
	ImVec2 canvasMax = ImVec2(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);

	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 30, 255));

	float gridSpacing = 25.0f;
	ImU32 gridCol = IM_COL32(50, 50, 50, 255);
	for (float x = canvasMin.x; x < canvasMax.x; x += gridSpacing)
		drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), gridCol);
	for (float y = canvasMin.y; y < canvasMax.y; y += gridSpacing)
		drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), gridCol);

	ImGui::InvisibleButton("##animCanvas", canvasSize);

	for (const auto& t : m_Transitions)
	{
		glm::vec2 fromPos(0), toPos(0);
		for (const auto& s : m_States)
		{
			if (s.Name == t.FromState) fromPos = s.Position;
			if (s.Name == t.ToState) toPos = s.Position;
		}
		if (fromPos.x == 0 && fromPos.y == 0) continue;
		if (toPos.x == 0 && toPos.y == 0) continue;

		ImVec2 p1(canvasMin.x + fromPos.x, canvasMin.y + fromPos.y);
		ImVec2 p2(canvasMin.x + toPos.x, canvasMin.y + toPos.y);

		ImVec2 mid(ImLerp(p1.x, p2.x, 0.5f), ImLerp(p1.y, p2.y, 0.5f));

		ImU32 arrowCol = IM_COL32(200, 200, 100, 255);
		ImU32 bezierCol = IM_COL32(180, 180, 100, 200);

		float dx = p2.x - p1.x;
		float dy = p2.y - p1.y;
		float dist = sqrtf(dx * dx + dy * dy);
		float perpOffset = (dist > 0) ? 30.0f : 0.0f;
		ImVec2 perp(-dy / dist * perpOffset, dx / dist * perpOffset);

		ImVec2 cp1(mid.x + perp.x, mid.y + perp.y);
		ImVec2 cp2(mid.x + perp.x, mid.y + perp.y);

		drawList->AddBezierCurve(p1, cp1, cp2, p2, bezierCol, 2.0f);

		float angle = atan2f(dy, dx);
		float arrowSize = 10.0f;
		ImVec2 arrow1(p2.x - arrowSize * cosf(angle - 0.5f), p2.y - arrowSize * sinf(angle - 0.5f));
		ImVec2 arrow2(p2.x - arrowSize * cosf(angle + 0.5f), p2.y - arrowSize * sinf(angle + 0.5f));
		drawList->AddTriangleFilled(p2, arrow1, arrow2, arrowCol);
	}

	for (auto& s : m_States)
	{
		const float stateW = 150.0f;
		const float stateH = 50.0f;
		ImVec2 statePos(canvasMin.x + s.Position.x, canvasMin.y + s.Position.y);

		bool isSelected = (s.Name == m_SelectedState);
		ImU32 bgCol = isSelected ? IM_COL32(70, 130, 200, 255) : IM_COL32(60, 60, 80, 255);
		ImU32 borderCol = isSelected ? IM_COL32(100, 180, 255, 255) : IM_COL32(100, 100, 120, 255);

		drawList->AddRectFilled(statePos, ImVec2(statePos.x + stateW, statePos.y + stateH), bgCol, 6.0f);
		drawList->AddRect(statePos, ImVec2(statePos.x + stateW, statePos.y + stateH), borderCol, 6.0f);

		ImGui::SetCursorScreenPos(statePos);
		ImGui::PushID(s.Name.c_str());
		if (ImGui::InvisibleButton("##stateBtn", ImVec2(stateW, stateH)))
		{
			m_SelectedState = s.Name;
		}
		ImGui::PopID();

		ImGui::SetCursorScreenPos(ImVec2(statePos.x + 8, statePos.y + 6));
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", s.Name.c_str());

		if (!s.ClipPath.empty())
		{
			std::string clipName = std::filesystem::path(s.ClipPath).filename().string();
			ImGui::SetCursorScreenPos(ImVec2(statePos.x + 8, statePos.y + 22));
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "%s", clipName.c_str());
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(canvasMin.x + 10, canvasMin.y + 10));
	if (ImGui::Button("+ Add State"))
	{
		ImGui::OpenPopup("AddStatePopup");
	}

	if (ImGui::BeginPopup("AddStatePopup"))
	{
		static char buf[64] = "NewState";
		ImGui::InputText("State Name", buf, sizeof(buf));
		if (ImGui::Button("Create"))
		{
			glm::vec2 newPos(100.0f + (float)(m_States.size() % 5) * 180.0f, 100.0f + (float)(m_States.size() / 5) * 80.0f);
			AddState(buf, newPos);
			memset(buf, 0, sizeof(buf));
			strcpy(buf, "NewState");
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void AnimatorEditorPanel::AddState(const std::string& name, const glm::vec2& pos)
{
	AnimatorEditorState s;
	s.Name = name;
	s.Position = pos;
	m_States.push_back(s);
}

void AnimatorEditorPanel::RemoveState(const std::string& name)
{
	m_States.erase(
		std::remove_if(m_States.begin(), m_States.end(),
			[&name](const AnimatorEditorState& s) { return s.Name == name; }),
		m_States.end());
	m_Transitions.erase(
		std::remove_if(m_Transitions.begin(), m_Transitions.end(),
			[&name](const AnimatorEditorTransition& t) { return t.FromState == name || t.ToState == name; }),
		m_Transitions.end());
	if (m_SelectedState == name)
		m_SelectedState.clear();
}

void AnimatorEditorPanel::RenameState(const std::string& oldName, const std::string& newName)
{
	for (auto& s : m_States)
	{
		if (s.Name == oldName)
		{
			s.Name = newName;
			break;
		}
	}
	for (auto& t : m_Transitions)
	{
		if (t.FromState == oldName) t.FromState = newName;
		if (t.ToState == oldName) t.ToState = newName;
	}
	if (m_SelectedState == oldName)
		m_SelectedState = newName;
}

void AnimatorEditorPanel::AddTransition(const std::string& from, const std::string& to)
{
	for (const auto& t : m_Transitions)
	{
		if (t.FromState == from && t.ToState == to)
			return;
	}
	AnimatorEditorTransition t;
	t.FromState = from;
	t.ToState = to;
	t.Duration = 0.0f;
	t.ExitTime = 0.5f;
	t.HasExitTime = true;
	m_Transitions.push_back(t);
}

void AnimatorEditorPanel::RemoveTransition(const std::string& from, const std::string& to)
{
	m_Transitions.erase(
		std::remove_if(m_Transitions.begin(), m_Transitions.end(),
			[&from, &to](const AnimatorEditorTransition& t) { return t.FromState == from && t.ToState == to; }),
		m_Transitions.end());
}

void AnimatorEditorPanel::AddParameter(const AnimatorParameter& param)
{
	m_Parameters.push_back(param);
}

void AnimatorEditorPanel::RemoveParameter(const std::string& name)
{
	m_Parameters.erase(
		std::remove_if(m_Parameters.begin(), m_Parameters.end(),
			[&name](const AnimatorParameter& p) { return p.Name == name; }),
		m_Parameters.end());
}

AnimatorParameter* AnimatorEditorPanel::GetParameter(const std::string& name)
{
	for (auto& p : m_Parameters)
	{
		if (p.Name == name)
			return &p;
	}
	return nullptr;
}

void AnimatorEditorPanel::StartTransition(const std::string& from, const std::string& to)
{
	m_EditingTransitionFrom = from;
	m_EditingTransitionTo = to;
	m_NewTransitionDuration = 0.0f;
	m_NewTransitionExitTime = 0.5f;
	m_NewTransitionHasExitTime = true;
}

void AnimatorEditorPanel::CancelTransition()
{
	m_EditingTransitionFrom.clear();
	m_EditingTransitionTo.clear();
}

void AnimatorEditorPanel::FinishTransition()
{
	for (auto& t : m_Transitions)
	{
		if (t.FromState == m_EditingTransitionFrom && t.ToState == m_EditingTransitionTo)
		{
			t.Duration = m_NewTransitionDuration;
			t.ExitTime = m_NewTransitionExitTime;
			t.HasExitTime = m_NewTransitionHasExitTime;
			break;
		}
	}
	CancelTransition();
}

}
