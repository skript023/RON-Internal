#pragma once
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"

namespace big::features
{
	inline bool_command _esp_enabled("esp_activate", "Enable ESP", "Enable ESP", false);
	inline bool_command _draw_line("draw_line", "ESP Line", "ESP must be enabled", false);
	inline bool_command _draw_name("draw_name", "ESP Name", "ESP must be enabled", false);
	inline bool_command _draw_skeleton("draw_skeleton", "ESP Skeleton", "ESP must be enabled", false);
	inline bool_command _draw_health("draw_health", "ESP Health Bar", "ESP must be enabled", false);
	inline bool_command _draw_box("draw_box", "ESP Box", "ESP must be enabled", false);
	inline bool_command _draw_corner_box("draw_corner_box", "ESP Corner Box", "ESP must be enabled", false);
	inline bool_command _draw_box_3d("draw_box_3d", "ESP Box 3D", "ESP must be enabled", false);
	inline bool_command _draw_fov("draw_fov", "Draw FOV", "Draw max aim angle", false);

	// What the ESP shows.
	inline bool_command _esp_suspects("esp_suspects", "Show Suspects", "Armed and unarmed suspects", true);
	inline bool_command _esp_civilians("esp_civilians", "Show Civilians", "Civilians and hostages", true);
	inline bool_command _esp_swat("esp_swat", "Show SWAT", "Your AI squad", false);
	inline bool_command _esp_traps("esp_traps", "Show Traps", "Live door and tripwire traps", true);
	inline bool_command _esp_evidence("esp_evidence", "Show Evidence", "Dropped weapons and evidence still to collect", true);
	inline bool_command _esp_reportables("esp_reportables", "Show Reportables", "Things still to report to TOC", false);
	inline bool_command _esp_collectibles("esp_collectibles", "Show Collectibles", "Lore collectibles on the map", false);
	inline number_command<float> _esp_distance("esp_distance", "Max Distance", "Things further away than this are not drawn", 5.f, 500.f, 150.f);
}
