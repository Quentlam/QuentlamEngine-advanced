#include "qlpch.h"
#include "UIRenderer.h"
#include "Quentlam/Renderer/Renderer2D.h"
#include "Quentlam/Core/Log.h"
#include <imgui.h>

namespace Quentlam
{

UIRenderer& UIRenderer::Get()
{
	static UIRenderer instance;
	return instance;
}

void UIRenderer::Begin(int screenWidth, int screenHeight)
{
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;
	m_QuadBuffer.clear();
	m_TextBuffer.clear();
}

void UIRenderer::End()
{
	Flush();
}

void UIRenderer::DrawQuad(const UIQuad& quad)
{
	m_QuadBuffer.push_back(quad);
}

void UIRenderer::DrawText(const UITextData& text)
{
	m_TextBuffer.push_back(text);
}

void UIRenderer::Flush()
{
	for (const auto& quad : m_QuadBuffer)
	{
		if (quad.Texture)
		{
			Renderer2D::DrawRotatedQuad(
				quad.Position,
				quad.Size,
				quad.Texture,
				0.0f,
				quad.TilingFactor,
				quad.Color
			);
		}
		else
		{
			Renderer2D::DrawQuad(
				quad.Position,
				quad.Size,
				quad.Color
			);
		}
	}
	m_QuadBuffer.clear();

	for (const auto& tq : m_TextBuffer)
	{
		ImGui::SetNextWindowPos(ImVec2(tq.Position.x, tq.Position.y));
		ImGui::SetNextWindowSize(ImVec2(tq.Size.x, tq.Size.y));
		ImGui::Begin("##UIText", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoInputs);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(tq.Color.r, tq.Color.g, tq.Color.b, tq.Color.a));
		ImGui::SetWindowFontScale(tq.FontSize / 16.0f);
		ImGui::Text("%s", tq.Text.c_str());
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor();
		ImGui::End();
	}
	m_TextBuffer.clear();
}

}
