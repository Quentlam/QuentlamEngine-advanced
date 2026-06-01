#include "qlpch.h"
#include "Quentlam/Gameplay/AnimationModule.h"
#include "Quentlam/Gameplay/AnimationLoader.h"
#include "Quentlam/Scene/Scene.h"
#include <algorithm>
#include <cmath>

namespace Quentlam
{

// ============================================================
// AnimationClip
// ============================================================

std::string AnimationClip::SerializeToJson() const
{
	return "{}";
}

bool AnimationClip::DeserializeFromJson(const std::string& json)
{
	return true;
}

// ============================================================
// Animator
// ============================================================

// ============================================================
// AnimationStateMachine
// ============================================================
// (all methods are inline in header)

}
