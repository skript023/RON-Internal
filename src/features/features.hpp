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
	inline bool_command _draw_box_3d("draw_box_3d", "ESP Box 3D", "ESP must be enabled", false);
	inline bool_command _draw_team("draw_team", "ESP Team", "ESP must be enabled", false);
	inline bool_command _draw_fov("draw_fov", "Draw FOV", "Draw max aim angle", false);
	inline bool_command _draw_anim("draw_anim", "Draw Animation", "Draw animation", false);
}