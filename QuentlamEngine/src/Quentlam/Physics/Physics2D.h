#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Scene/Scene.h"
#include <vector>
#include <glm/glm.hpp>

namespace Quentlam {

struct RaycastHit
{
	glm::vec2 Point;
	glm::vec2 Normal;
	float Distance;
	uint32_t EntityId;
};

class QUENTLAM_API Physics2D
{
public:
	static bool OnRuntimeStart(Scene* scene);
	static void OnRuntimeStop(Scene* scene);
	static void OnUpdate(Scene* scene, Timestep ts);

	static bool Raycast(const glm::vec2& origin, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit);
	static bool RaycastAll(const glm::vec2& origin, const glm::vec2& direction, float distance, uint32_t layerMask, std::vector<RaycastHit>& outHits);
	static bool BoxCast(const glm::vec2& center, const glm::vec2& halfExtents, float angle, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit);
	static bool CircleCast(const glm::vec2& center, float radius, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit);
	static bool OverlapPoint(const glm::vec2& point, uint32_t layerMask);
	static bool OverlapBox(const glm::vec2& center, const glm::vec2& halfExtents, float angle, uint32_t layerMask);
	static bool OverlapCircle(const glm::vec2& center, float radius, uint32_t layerMask);
};

}
