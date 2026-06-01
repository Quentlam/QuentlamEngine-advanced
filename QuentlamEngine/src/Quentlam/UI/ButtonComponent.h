#pragma once
#include <string>

namespace Quentlam
{

enum class EButtonTransition
{
	ColorTint,
	SpriteSwap,
	Animation
};

struct ButtonComponent
{
	std::string NormalSpritePath;
	std::string HighlightedSpritePath;
	std::string PressedSpritePath;
	std::string DisabledSpritePath;
	float TransitionDuration = 0.1f;
	EButtonTransition Transition = EButtonTransition::ColorTint;
	std::string OnClickLuaFunction;
	std::string OnPointerEnterLuaFunction;
	std::string OnPointerExitLuaFunction;
};

}
