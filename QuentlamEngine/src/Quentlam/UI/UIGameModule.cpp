#include "qlpch.h"
#include "Quentlam/UI/UIGameModule.h"
#include "Quentlam/Core/Log.h"
#include "Quentlam/Audio/AudioModule.h"

namespace Quentlam
{

void UIGameModule::PlayUISound(EUISound sound)
{
	if (!m_UIEnabled || sound == EUISound::None)
		return;

	if (OnPlaySound)
		OnPlaySound(sound);
}

}
