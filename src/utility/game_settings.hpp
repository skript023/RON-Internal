#pragma once

#include "pointers.hpp"
#include "unreal_engine_utility.hpp"

namespace big::game_setting
{
	inline void fps_unlock()
	{
		static bool bEnabled = false;

		SDK::UEngine* Engine = g_pointers->m_engine;

		if (!Engine) return;

		SDK::UGameUserSettings* Settings = Engine->GameUserSettings;

		if (!Settings) return;

		if (g_settings.game_setting.fps_unlock)
		{
			if (Settings->GetFrameRateLimit() == g_settings.game_setting.fps_max)
				return;

			Settings->SetFrameRateLimit(g_settings.game_setting.fps_max);
			Settings->ApplySettings(false);
			bEnabled = true;
		}
		else
		{
			if (bEnabled)
			{
				Settings->FrameRateLimit = 120.f;
				Settings->ApplySettings(false);
				bEnabled = false;
			}
		}
	}
}