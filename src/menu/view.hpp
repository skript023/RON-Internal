#pragma once
#include "imgui.h"

namespace big
{
	class view
	{
	public:
		static void register_submenu()
		{
			player_submenu();
			mission_submenu();
			world_submenu();
			esp_submenu();
			aimbot_submenu();
			teleport_submenu();
			online_players_submenu();
			achievement_submenu();
			setting_submenu();
		}
	public:
		// Drawn every frame, menu open or not.
		static void draw_input();
		static void notifications();
		static void mission_overlay();
		static void target_inspector();
		static void collectibles_panel();
		static void bookmarks_panel();
		static void career_stats_panel();
		static void give_item_panel();
	public:
		static void esp_submenu();
		static void player_submenu();
		static void mission_submenu();
		static void world_submenu();
		static void aimbot_submenu();
		static void online_players_submenu();
		static void achievement_submenu();
		static void teleport_submenu();
		static void setting_submenu();
	};
}
