#include "qlpch.h"
#include "EditorCommand.h"
#include "Quentlam/Scene/Scene.h"
#include "Quentlam/Scene/Entity.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{
	// =========================================================================
	// EditorCommandStack
	// =========================================================================

	void EditorCommandStack::Execute(std::shared_ptr<ICommand> command)
	{
		command->Execute();
		m_UndoStack.push(command);
		std::stack<std::shared_ptr<ICommand>> empty;
		std::swap(m_RedoStack, empty);
	}

	void EditorCommandStack::Undo()
	{
		if (m_UndoStack.empty())
			return;
		auto command = m_UndoStack.top();
		m_UndoStack.pop();
		command->Undo();
		m_RedoStack.push(command);
	}

	void EditorCommandStack::Redo()
	{
		if (m_RedoStack.empty())
			return;
		auto command = m_RedoStack.top();
		m_RedoStack.pop();
		command->Execute();
		m_UndoStack.push(command);
	}

	void EditorCommandStack::Clear()
	{
		std::stack<std::shared_ptr<ICommand>> empty;
		std::swap(m_UndoStack, empty);
		std::swap(m_RedoStack, empty);
	}

	// =========================================================================
	// CreateEntityCommand
	// =========================================================================

	CreateEntityCommand::CreateEntityCommand(Scene* scene, const std::string& tag, Entity* outEntity)
		: m_Scene(scene), m_Tag(tag), m_OutEntity(outEntity)
	{
	}

	void CreateEntityCommand::Execute()
	{
		if (!m_Scene)
			return;
		Entity e = m_Scene->CreateEntity(m_Tag);
		m_Entity = static_cast<entt::entity>(e);
		if (m_OutEntity)
			*m_OutEntity = e;
	}

	void CreateEntityCommand::Undo()
	{
		if (!m_Scene || m_Entity == entt::null)
			return;
		m_Scene->DestroyEntity(m_Entity);
	}

	// =========================================================================
	// DeleteEntityCommand
	// =========================================================================

	DeleteEntityCommand::DeleteEntityCommand(Scene* scene, entt::entity entity)
		: m_Scene(scene), m_Entity(entity)
	{
		if (!m_Scene)
			return;
		auto& reg = m_Scene->GetRegistry();
		if (!reg.valid(entity))
			return;
		if (auto* tag = reg.try_get<TagComponent>(entity))
			m_Tag = tag->Tag;
	}

	void DeleteEntityCommand::Execute()
	{
		if (!m_Scene || m_Entity == entt::null)
			return;
		m_Scene->DestroyEntity(m_Entity);
	}

	void DeleteEntityCommand::Undo()
	{
		if (!m_Scene)
			return;
		m_Scene->CreateEntity(m_Tag);
	}

	// =========================================================================
	// TransformCommand
	// =========================================================================

	TransformCommand::TransformCommand(Scene* scene, entt::entity entity,
		const glm::mat4& beforeTransform,
		const glm::mat4& afterTransform)
		: m_Scene(scene), m_Entity(entity),
		m_BeforeTransform(beforeTransform),
		m_AfterTransform(afterTransform)
	{
	}

	void TransformCommand::Execute()
	{
		if (!m_Scene || m_Entity == entt::null)
			return;
		auto& reg = m_Scene->GetRegistry();
		if (!reg.valid(m_Entity) || !reg.all_of<TransformComponent>(m_Entity))
			return;
		reg.get<TransformComponent>(m_Entity).Transform = m_AfterTransform;
	}

	void TransformCommand::Undo()
	{
		if (!m_Scene || m_Entity == entt::null)
			return;
		auto& reg = m_Scene->GetRegistry();
		if (!reg.valid(m_Entity) || !reg.all_of<TransformComponent>(m_Entity))
			return;
		reg.get<TransformComponent>(m_Entity).Transform = m_BeforeTransform;
	}

	// =========================================================================
	// BatchCommand
	// =========================================================================

	BatchCommand::BatchCommand(const std::string& name,
		const std::vector<std::shared_ptr<ICommand>>& commands)
		: m_Name(name), m_Commands(commands)
	{
	}

	void BatchCommand::Execute()
	{
		for (auto& cmd : m_Commands)
			if (cmd) cmd->Execute();
	}

	void BatchCommand::Undo()
	{
		for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it)
			if (*it) (*it)->Undo();
	}
}
