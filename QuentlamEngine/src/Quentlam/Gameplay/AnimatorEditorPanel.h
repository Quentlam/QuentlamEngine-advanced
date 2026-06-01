#pragma once

#include "Quentlam/Core/Base.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Quentlam
{

class AnimatorController;
class AnimationClip;

struct AnimatorParameter
{
	enum class EType { Float, Int, Bool, Trigger };
	std::string Name;
	EType Type = EType::Float;
	float DefaultFloat = 0.0f;
	int32_t DefaultInt = 0;
	bool DefaultBool = false;
};

struct AnimatorEditorState
{
	std::string Name;
	std::string ClipPath;
	std::string ColorHex = "#4488FF";
	glm::vec2 Position = { 0.0f, 0.0f };
	float Speed = 1.0f;
	std::vector<std::string> Transitions;
};

struct AnimatorEditorTransition
{
	std::string FromState;
	std::string ToState;
	std::vector<std::pair<std::string, std::string>> Conditions;
	float Duration = 0.0f;
	float ExitTime = 0.0f;
	bool HasExitTime = false;
	bool HasFixedDuration = true;
	glm::vec2 ControlPoint = { 0.0f, 0.0f };
};

class AnimatorEditorPanel
{
public:
	static AnimatorEditorPanel& Get();

	void OnImGuiRender();

	bool IsOpen() const { return m_IsOpen; }
	void Open() { m_IsOpen = true; }
	void Close() { m_IsOpen = false; }
	void Toggle() { m_IsOpen = !m_IsOpen; }

	void SetController(Ref<AnimatorController> controller) { m_Controller = controller; }
	Ref<AnimatorController> GetController() const { return m_Controller; }

	void AddState(const std::string& name, const glm::vec2& pos);
	void RemoveState(const std::string& name);
	void RenameState(const std::string& oldName, const std::string& newName);

	void AddTransition(const std::string& from, const std::string& to);
	void RemoveTransition(const std::string& from, const std::string& to);

	const std::string& GetSelectedState() const { return m_SelectedState; }
	void SelectState(const std::string& name) { m_SelectedState = name; }

	const std::string& GetEditingTransitionFrom() const { return m_EditingTransitionFrom; }
	const std::string& GetEditingTransitionTo() const { return m_EditingTransitionTo; }
	void StartTransition(const std::string& from, const std::string& to);
	void CancelTransition();
	void FinishTransition();

	bool IsDraggingState() const { return !m_DraggingStateName.empty(); }
	const std::string& GetDraggingStateName() const { return m_DraggingStateName; }
	void StartDraggingState(const std::string& name) { m_DraggingStateName = name; }
	void StopDraggingState() { m_DraggingStateName.clear(); }

	bool IsTransitioning() const { return !m_EditingTransitionFrom.empty(); }

	std::vector<AnimatorParameter>& GetParameters() { return m_Parameters; }
	void AddParameter(const AnimatorParameter& param);
	void RemoveParameter(const std::string& name);
	AnimatorParameter* GetParameter(const std::string& name);

private:
	AnimatorEditorPanel() = default;
	~AnimatorEditorPanel() = default;

	void RenderParametersPanel();
	void RenderStateMachineCanvas();
	void RenderStateProperties();
	void RenderStateContextMenu(const glm::vec2& clickPos);
	void RenderTransitionPopup();

	bool m_IsOpen = false;
	Ref<AnimatorController> m_Controller;

	std::vector<AnimatorEditorState> m_States;
	std::vector<AnimatorEditorTransition> m_Transitions;
	std::vector<AnimatorParameter> m_Parameters;

	std::string m_SelectedState;
	std::string m_DraggingStateName;
	glm::vec2 m_LastRightClickPos = { 0.0f, 0.0f };

	std::string m_EditingTransitionFrom;
	std::string m_EditingTransitionTo;
	float m_NewTransitionDuration = 0.0f;
	float m_NewTransitionExitTime = 0.0f;
	bool m_NewTransitionHasExitTime = true;
	std::string m_NewConditionParam;
	std::string m_NewConditionMode = "Equals";

	std::string m_NewStateName = "NewState";
	std::string m_RenamingState;
	std::string m_RenameBuffer = "";

	glm::vec2 m_CanvasOffset = { 0.0f, 0.0f };
	float m_CanvasScale = 1.0f;
	glm::vec2 m_GridSize = { 50.0f, 50.0f };
};

}
