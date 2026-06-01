#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/SpriteAtlasPacker.h"
#include "Quentlam/Scene/Scene.h"
#include <string>
#include <vector>

namespace Quentlam
{

class AtlasBuilderPanel
{
public:
	AtlasBuilderPanel() = default;

	void OnImGuiRender();
	void SetScene(Scene* scene) { m_Scene = scene; }

private:
	void OpenFolder();
	void ClearImages();
	void PackAtlas();
	void SaveAtlas();

	Scene* m_Scene = nullptr;
	SpriteAtlasPacker m_Packer;
	std::string m_OutputAtlasPath;
	std::string m_OutputMetaPath;
	bool m_AtlasGenerated = false;
};

}
