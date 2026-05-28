#include "qlpch.h"
#include "UIHierarchyPanel.h"
#include "Quentlam/UI/UIGameModule.h"
#include <imgui.h>

namespace Quentlam
{
	UIHierarchyPanel::UIHierarchyPanel()
	{
	}

	void UIHierarchyPanel::SetScreenStack(ScreenStack* stack)
	{
		m_ScreenStack = stack;
		RefreshHierarchy();
	}

	void UIHierarchyPanel::SetUIGameModule(UIGameModule* module)
	{
		m_UIGameModule = module;
		if (m_UIGameModule)
			m_ScreenStack = &m_UIGameModule->GetScreenStack();
		RefreshHierarchy();
	}

	void UIHierarchyPanel::RefreshHierarchy()
	{
		m_NeedsRefresh = true;
	}

	void UIHierarchyPanel::OnImGuiRender()
	{
		if (ImGui::Begin("UI 层级"))
		{
			// Always rebuild for real-time sync
			BuildTree();

			if (ImGui::Button("+ 新建 Screen", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				ImGui::OpenPopup("CreateUIScreen");
			}
			RenderCreateMenu();

			ImGui::Separator();

			if (m_RootNodes.empty())
			{
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No screens active.");
				if (m_UIGameModule)
				{
					auto& registry = m_UIGameModule->GetRegisteredScreens();
					if (!registry.empty())
					{
						ImGui::Spacing();
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Registered screens (%zu):", registry.size());
						for (auto& [id, screen] : registry)
						{
							ImGui::Text("  %s", id.c_str());
						}
						if (ImGui::Button("Push First Screen", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
						{
							if (!registry.empty())
							{
								auto& first = registry.begin()->second;
								m_UIGameModule->GetScreenStack().Push(first);
								RefreshHierarchy();
							}
						}
					}
					else
					{
						ImGui::Text("No registered screens.");
						ImGui::Text("Use UIGameModule::RegisterScreen()");
					}
				}
				else
				{
					ImGui::Text("UIGameModule not connected.");
				}
			}

			for (auto& node : m_RootNodes)
			{
				RenderTreeNode(node, 0);
			}
		}
		ImGui::End();
	}

	void UIHierarchyPanel::BuildTree()
	{
		m_RootNodes.clear();

		if (!m_ScreenStack) return;

		const auto& screens = m_ScreenStack->GetAll();
		for (const auto& screen : screens)
		{
			if (!screen) continue;

			TreeNode node;
			node.Name = screen->GetId();
			node.FullPath = screen->GetId();
			node.Screen = screen;
			node.Expanded = true;

			const auto& children = screen->GetWidgetTree().GetChildren();
			for (const auto& child : children)
			{
				if (!child) continue;
				TreeNode childNode;
				childNode.Name = child->GetId();
				childNode.FullPath = screen->GetId() + "/" + child->GetId();
				childNode.Screen = child;
				node.Children.push_back(childNode);
			}

			m_RootNodes.push_back(node);
		}
	}

	void UIHierarchyPanel::RenderTreeNode(TreeNode& node, int depth)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
		if (node.Children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (node.Expanded)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (!m_SelectedScreen.expired() && m_SelectedScreen.lock() == node.Screen)
			flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::Indent(static_cast<float>(depth) * 12.0f);

		bool opened = ImGui::TreeNodeEx(node.Name.c_str(), flags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			SetSelection(node.Screen);
			if (OnScreenSelected)
				OnScreenSelected(node.Screen);
		}

		if (opened)
		{
			node.Expanded = true;
			for (auto& child : node.Children)
			{
				RenderTreeNode(child, depth + 1);
			}
			ImGui::TreePop();
		}
		else
		{
			node.Expanded = false;
		}

		ImGui::Unindent(static_cast<float>(depth) * 12.0f);
	}

	void UIHierarchyPanel::RenderCreateMenu()
	{
		if (ImGui::BeginPopup("CreateUIScreen"))
		{
			ImGui::Text("Create New Screen");
			ImGui::Separator();

			static char nameBuf[128] = "NewScreen";
			ImGui::InputText("Screen ID", nameBuf, sizeof(nameBuf));

			if (ImGui::Button("Create", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
			{
				auto screen = CreateRef<UIScreen>(nameBuf);
				if (OnScreenCreated)
					OnScreenCreated(screen);
				RefreshHierarchy();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void UIHierarchyPanel::SetSelection(Ref<UIScreen> screen)
	{
		m_SelectedScreen = screen;
	}
}
