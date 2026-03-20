#pragma once
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"

namespace big::features
{
	static bool_command _esp_enabled("esp_activate", "Enable ESP", "Enable ESP", false);
	static bool_command _draw_line("draw_line", "ESP Line", "ESP must be enabled", false);
	static bool_command _draw_name("draw_name", "ESP Name", "ESP must be enabled", false);
	static bool_command _draw_skeleton("draw_skeleton", "ESP Skeleton", "ESP must be enabled", false);
	static bool_command _draw_health("draw_health", "ESP Health Bar", "ESP must be enabled", false);
	static bool_command _draw_box("draw_box", "ESP Box", "ESP must be enabled", false);
	static bool_command _draw_team("draw_team", "ESP Team", "ESP must be enabled", false);
	static bool_command _draw_fov("draw_fov", "Draw FOV", "Draw max aim angle", false);
}