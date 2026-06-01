#include "qlpch.h"
#include "Quentlam/Gameplay/LODComponent.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/Core/Log.h"

namespace Quentlam
{

void LODComponentUpdate(LODComponent& lod, const TransformComponent& transform, const glm::vec3& cameraPosition)
{
	if (!lod.AutoUpdate || lod.Levels.empty())
		return;

	glm::mat4 mat = transform.Transform;
	glm::vec3 worldPos = glm::vec3(mat[3]);
	float distance = glm::distance(worldPos, cameraPosition);

	int32_t newLevel = 0;
	for (int32_t i = 0; i < static_cast<int32_t>(lod.Levels.size()); ++i)
	{
		if (distance >= lod.Levels[i].Distance)
		{
			newLevel = i;
		}
		else
		{
			break;
		}
	}

	if (newLevel != lod.CurrentLevel)
	{
		QL_CORE_WARN("LOD: Switching from level {0} to level {1} (distance: {2})",
			lod.CurrentLevel, newLevel, distance);
		lod.CurrentLevel = newLevel;
	}
}

}
