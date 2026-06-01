#pragma once
#include "Quentlam/Core/Base.h"
#include <vector>

namespace Quentlam
{

struct LODLevel
{
	float Distance = 100.0f;
	std::string MeshPath;
	Ref<class Mesh> Mesh;
};

struct LODComponent
{
	std::vector<LODLevel> Levels;
	int32_t CurrentLevel = 0;
	bool AutoUpdate = true;
};

}
