#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/SubTexture2D.h"
#include <vector>
#include <glm/glm.hpp>

namespace Quentlam
{

struct UIQuad
{
	glm::vec3 Position = { 0, 0, 0 };
	glm::vec2 Size = { 100, 50 };
	glm::vec4 Color = { 1, 1, 1, 1 };
	Ref<SubTexture2D> Texture;
	float TilingFactor = 1.0f;
};

struct UITextData
{
	glm::vec2 Position;
	glm::vec2 Size;
	glm::vec4 Color;
	float FontSize;
	std::string Text;
};

class UIRenderer
{
public:
	static UIRenderer& Get();

	void Begin(int screenWidth, int screenHeight);
	void End();
	void Flush();

	void DrawQuad(const UIQuad& quad);
	void DrawText(const UITextData& text);

private:
	UIRenderer() = default;

	int m_ScreenWidth = 0;
	int m_ScreenHeight = 0;

	std::vector<UIQuad> m_QuadBuffer;
	std::vector<UITextData> m_TextBuffer;
};

}
