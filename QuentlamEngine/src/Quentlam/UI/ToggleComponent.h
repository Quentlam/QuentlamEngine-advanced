#pragma once
#include <string>

namespace Quentlam
{

struct ToggleComponent
{
	bool IsOn = false;
	std::string ToggleGroup;
	std::string OnValueChangedLuaFunction;
	std::string NormalSpritePath;
	std::string HighlightedSpritePath;
	std::string DisabledSpritePath;
};

}
