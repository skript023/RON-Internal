#pragma once

#include "imgui.h"
#include "logger/logger.hpp"
#include "class/vector.hpp"
#include "utility/utility.hpp"

namespace big
{
	class menu_settings 
	{
		nlohmann::json default_options;
		nlohmann::json options;
	public:
		void attempt_save();

		bool load();
	private:
		static std::filesystem::path get_file_path();

		bool deep_compare(nlohmann::json& current_settings, const nlohmann::json& default_settings, bool compare_value = false);
		bool save();

		bool write_default_config();
	public:
		struct esp
		{
			bool is_all_entity = false;
			bool is_active = true;
			bool draw_line = true;
			bool draw_name = true;
			bool draw_box = false;
			bool skeleton = false;
			bool health = false;
			bool team = false;
			bool draw_fov = false;
			float health_size = 1.f;
			float max_draw_distance = 200.f;
			float global_render_distance[2] = { 0.f, 200.f };
			float tracer_render_distance[2] = { 200.f, 200.f };
			float box_render_distance[2] = { 0.f, 150.f };
			float tracer_draw_position[2] = { 0.5f, 1.f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(esp, is_all_entity, is_active, draw_line, draw_name, draw_box, skeleton, health, team, draw_fov)
		} esp;

		struct game_setting
		{
			bool fps_unlock{ false };
			float fps_max{ 165 };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(game_setting, fps_unlock, fps_max)
		} game_setting;

		struct debug
		{
			bool external_console = true;
			bool logging_javascript = false;
			bool enable_devtools = false;
			bool logging_godmode = false;
			bool logging_process_event = false;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(debug, external_console, logging_javascript, enable_devtools)
		} debug;

		struct aimbot
		{
			float pitch{ 0.f };
			float yaw{ 0.f };
			bool has_target{ false };
			// The key Aimbot Trigger Type 2 holds. Saved but not in the mod menu: the Astra loader sets it.
			uint32_t trigger_key{ VK_XBUTTON1 };
			SDK::FVector original_position{ 0.f, 0.f, 0.f };
			SDK::FRotator original_rotation{ 0.f, 0.f, 0.f };
			std::vector<std::string> aimbot_targets = {
				"Head",
				"Neck",
				"Body",
				"Pelvis"
			};
			int aimbot_target{0};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(aimbot, pitch, yaw, trigger_key, aimbot_target, aimbot_targets)
		} aimbot;

		struct system
		{
			bool disable_anti_cheat = false;
			bool disable_heartbeat = true;
			bool enable_preload = false;
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(system, disable_anti_cheat, disable_heartbeat, enable_preload)
		} system;
	

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(menu_settings, debug, esp, game_setting, aimbot, system)
	};

	inline menu_settings g_settings{};
}