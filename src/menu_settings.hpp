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
		const char* settings_location = "\\" FOLDER_NAME "\\menu_settings.json";

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

		struct window
		{
			ImU32 color = 3357612055;
			float gui_scale = 1.f;

			ImFont* font_title = nullptr;
			ImFont* font_sub_title = nullptr;
			ImFont* font_small = nullptr;
			ImFont* font_icon = nullptr;

			bool switched_view = true;
			bool mouse_active = false;
			bool input = false;
			bool censor = true;
			bool overlay = false;

			uint32_t open_key{ VK_INSERT };
			uint32_t back_key{ VK_NUMPAD0 };
			uint32_t enter_key{ VK_NUMPAD5 };
			uint32_t up_key{ VK_NUMPAD8 };
			uint32_t down_key{ VK_NUMPAD2 };
			uint32_t left_key{ VK_NUMPAD4 };
			uint32_t right_key{ VK_NUMPAD6 };
			uint32_t left_tab_key{ VK_NUMPAD7 };
			uint32_t right_tab_key{ VK_NUMPAD9 };

			Vector2 m_pos = { 25.f, 25.f };
			float m_width = 450.f;
			std::size_t m_option_per_page = 11;
			bool m_sounds = true;
			// Input
			std::int32_t m_open_delay = 200;
			std::int32_t m_back_delay = 300;
			std::int32_t m_enter_delay = 300;
			std::int32_t m_vectical_delay = 120;
			std::int32_t m_horizontal_delay = 120;
			std::int32_t m_tabbar_switch = 200;

			// Submenu bar
			Color m_submenu_bar_background_color{ 24, 24, 24, 255 };
			Color m_submenu_bar_text_color{ 153, 153, 155, 255 };

			//Options
			Color m_toggle_on_color{ 255, 255, 255, 200 };
			Color m_toggle_off_color{ 0, 0, 0, 150 };
			Color m_submenu_rect_color{ 255, 255, 255, 180 };
			Color m_option_selected_text_color{ 0, 0, 0, 255 };
			Color m_option_unselected_text_color{ 255, 255, 255, 255 };
			Color m_option_selected_background_color{ 255, 255, 255, 200 };
			Color m_option_unselected_background_color{ 0, 0, 0, 150 };

			//Footer
			Color m_footer_background_color{ 0, 0, 0, 150 };

			//Tabbar
			Color m_tab_unselected_text_color{ 255, 255, 255, 255 };
			Color m_tab_selected_text_color{ 0, 0, 0, 255 };
			Color m_tab_unselected_color{ 0, 0, 0, 150 };
			Color m_tab_selected_color{ 255, 255, 255, 200 };

			//Description
			Color description_background_color{ 0, 0, 0, 150 };
			Color description_text_color{ 255, 255, 255, 255 };

			//Sliderbar
			Color m_slider_track_color = { 255, 255, 255, 255 };
			Color m_slider_knob_color = { 0, 0, 0, 255 };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(window, 
				color, 
				gui_scale, 
				mouse_active, 
				censor, 
				overlay,
				open_key, 
				back_key, 
				enter_key, 
				up_key, 
				down_key, 
				left_key, 
				right_key, 
				left_tab_key, 
				right_tab_key,
				m_pos,
				m_width,
				m_option_per_page,
				m_sounds,
				m_open_delay,
				m_enter_delay,
				m_back_delay,
				m_horizontal_delay,
				m_vectical_delay,
				m_tabbar_switch,
				m_submenu_bar_background_color,
				m_submenu_bar_text_color,
				m_toggle_on_color,
				m_toggle_off_color,
				m_submenu_rect_color,
				m_option_selected_background_color,
				m_option_unselected_background_color,
				m_option_selected_text_color,
				m_option_unselected_text_color,
				m_footer_background_color,
				m_tab_selected_color,
				m_tab_unselected_color,
				m_tab_selected_text_color,
				m_tab_unselected_text_color,
				description_background_color,
				description_text_color,
				m_slider_track_color,
				m_slider_knob_color
			)
		} window;

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

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(aimbot, pitch, yaw, aimbot_target, aimbot_targets)
		} aimbot;

		struct system
		{
			bool disable_anti_cheat = false;
			bool disable_heartbeat = true;
			bool enable_preload = false;
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(system, disable_anti_cheat, disable_heartbeat, enable_preload)
		} system;
	

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(menu_settings, window, debug, esp, game_setting, aimbot, system)
	};

	inline menu_settings g_settings{};
}