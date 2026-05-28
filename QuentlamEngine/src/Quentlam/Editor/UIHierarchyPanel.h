#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/UI/UIGameModule.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Quentlam
{
	class UIHierarchyPanel
	{
	public:
		UIHierarchyPanel();
		~UIHierarchyPanel() = default;

		void SetScreenStack(ScreenStack* stack);
		void SetUIGameModule(UIGameModule* module);
		void OnImGuiRender();

		void SetSelection(Ref<UIScreen> screen);
		Ref<UIScreen> GetSelection() const { return m_SelectedScreen.lock(); }
		void ClearSelection() { m_SelectedScreen.reset(); }

		void RefreshHierarchy();

		std::function<void(Ref<UIScreen>)> OnScreenSelected;
		std::function<void(Ref<UIScreen>)> OnScreenCreated;
		std::function<void(Ref<UIScreen>)> OnScreenDeleted;
		std::function<void(Ref<UIScreen>, Ref<UIScreen>)> OnScreenDuplicated;

	private:
		struct TreeNode
		{
			std::string Name;
			std::string FullPath;
			Ref<UIScreen> Screen;
			std::vector<TreeNode> Children;
			bool Expanded = false;
		};

		void BuildTree();
		void RenderTreeNode(TreeNode& node, int depth);
		void RenderContextMenu(TreeNode& node);
		void RenderCreateMenu();

		ScreenStack* m_ScreenStack = nullptr;
		UIGameModule* m_UIGameModule = nullptr;
		std::vector<TreeNode> m_RootNodes;
		std::weak_ptr<UIScreen> m_SelectedScreen;
		bool m_NeedsRefresh = true;

		Ref<UIScreen> m_RenameTarget;
		char m_RenameBuffer[128] = { 0 };
		bool m_IsRenaming = false;

		Ref<UIScreen> m_CreateParent;
	};
}
