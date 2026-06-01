#pragma once
#include "Quentlam/Core/Base.h"
#include <entt/entt.hpp>
#include "Quentlam/Scene/Components.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <stack>
#include <memory>

namespace Quentlam
{
	class Entity;
	class Scene;

	class ICommand
	{
	public:
		virtual ~ICommand() = default;
		virtual void Execute() = 0;
		virtual void Undo() = 0;
		virtual std::string GetName() const = 0;
	};

	class EditorCommandStack
	{
	public:
		EditorCommandStack() = default;

		void Execute(std::shared_ptr<ICommand> command);
		void Undo();
		void Redo();

		bool CanUndo() const { return !m_UndoStack.empty(); }
		bool CanRedo() const { return !m_RedoStack.empty(); }

		void Clear();

	private:
		std::stack<std::shared_ptr<ICommand>> m_UndoStack;
		std::stack<std::shared_ptr<ICommand>> m_RedoStack;
	};

	class CreateEntityCommand : public ICommand
	{
	public:
		CreateEntityCommand(Scene* scene, const std::string& tag, Entity* outEntity);
		~CreateEntityCommand() override = default;

		void Execute() override;
		void Undo() override;
		std::string GetName() const override { return "Create Entity"; }

		entt::entity GetEntity() const { return m_Entity; }

	private:
		Scene* m_Scene = nullptr;
		std::string m_Tag;
		entt::entity m_Entity = entt::null;
		Entity* m_OutEntity = nullptr;
	};

	class DeleteEntityCommand : public ICommand
	{
	public:
		DeleteEntityCommand(Scene* scene, entt::entity entity);
		~DeleteEntityCommand() override = default;

		void Execute() override;
		void Undo() override;
		std::string GetName() const override { return "Delete Entity"; }

	private:
		Scene* m_Scene = nullptr;
		entt::entity m_Entity = entt::null;
		std::string m_SerializedData;
		std::string m_Tag;
	};

	class TransformCommand : public ICommand
	{
	public:
		TransformCommand(Scene* scene, entt::entity entity,
			const glm::mat4& beforeTransform,
			const glm::mat4& afterTransform);

		void Execute() override;
		void Undo() override;
		std::string GetName() const override { return "Transform Entity"; }

	private:
		Scene* m_Scene = nullptr;
		entt::entity m_Entity = entt::null;
		glm::mat4 m_BeforeTransform;
		glm::mat4 m_AfterTransform;
	};

	class BatchCommand : public ICommand
	{
	public:
		BatchCommand(const std::string& name, const std::vector<std::shared_ptr<ICommand>>& commands);
		~BatchCommand() override = default;

		void Execute() override;
		void Undo() override;
		std::string GetName() const override { return m_Name; }

	private:
		std::string m_Name;
		std::vector<std::shared_ptr<ICommand>> m_Commands;
	};
}
