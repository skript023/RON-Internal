#pragma once
#include "imgui.h"

namespace big
{
	class view
	{
	public:
		static void register_submenu()
		{
			//player_submenu();
			esp_submenu();
			aimbot_submenu();
			setting_submenu();
		}
	public:
		static void home();
		static void censor();
		static void draw_input();
		static void draw_overlay();
		static void notifications();
	public:
		static void esp_submenu();
		static void player_submenu();
		static void aimbot_submenu();
		static void teleport_submenu();
		static void setting_submenu();
	};
}