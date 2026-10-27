#include "common.hpp"
#include "gui.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <imgui.h>
#include <astra/banners.hpp>
#include <astra/host/canvas.hpp>
#include <astra/host/notification.hpp>
#include <astra/key_settings.hpp>

#include "menu/view.hpp"
#include "menu/esp/esp.h"

#include <input/input_service.hpp>

namespace big
{
	/**
	 * @brief The later an entry comes in this enum to higher up it comes in the z-index.
	 */
	enum eRenderPriority
	{
		// low priority
		ESP,

		// medium priority
		MENU = 0x1000,

		// high priority
		INFO_OVERLAY = 0x2000,
		INSPECTOR,
		COLLECTIBLES,
		BOOKMARKS,
		GIVE_ITEM,
		INPUT,

		// should remain in a league of its own
		NOTIFICATIONS = 0x4000,
	};

	void gui::init()
	{
		this->add_dx_callback(esp::draw_esp, eRenderPriority::ESP);
		this->add_dx_callback(view::mission_overlay, eRenderPriority::INFO_OVERLAY);
		this->add_dx_callback(view::target_inspector, eRenderPriority::INSPECTOR);
		this->add_dx_callback(view::collectibles_panel, eRenderPriority::COLLECTIBLES);
		this->add_dx_callback(view::bookmarks_panel, eRenderPriority::BOOKMARKS);
		this->add_dx_callback(view::give_item_panel, eRenderPriority::GIVE_ITEM);
		this->add_dx_callback(view::draw_input, eRenderPriority::INPUT);
		this->add_dx_callback(view::notifications, eRenderPriority::NOTIFICATIONS);
		this->add_dx_callback([this] { this->dx_on_opened(); }, eRenderPriority::MENU);

		this->add_wndproc_callback([this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { wndproc(hwnd, msg, wparam, lparam); });

		view::register_submenu();
		LOG(INFO) << "DirectX Callback Registered.";
	}
	void gui::dx_init()
	{
		// Sizes only: canvas::apply_panel_theme gives every window the menu's palette each frame.
		auto& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(15, 15);
		style.WindowBorderSize = 0.f;
		style.FramePadding = ImVec2(5, 5);
		style.ItemSpacing = ImVec2(12, 8);
		style.ItemInnerSpacing = ImVec2(8, 6);
		style.IndentSpacing = 25.0f;
		style.ScrollbarSize = 15.0f;
		style.GrabMinSize = 5.0f;
	}

	bool gui::add_dx_callback(dx_callback callback, uint32_t priority)
	{
		if (!m_dx_callbacks.insert({ priority, callback }).second)
		{
			LOG(INFO) << "Duplicate priority given on DX Callback!";

			return false;
		}
		return true;
	}

	void gui::add_wndproc_callback(wndproc_callback&& callback)
	{
		m_wndproc_callbacks.emplace_back(std::move(callback));
	}

	void gui::dx_on_opened()
	{
		// The banner index can change from the Settings tab or from settings reloaded on disk.
		update_banner();
		canvas::tick();
	}

	void gui::script_init()
	{
		notification::success("Welcome", std::format("{} trainer injected. Press {} to open the menu.", GAME_NAME, astra::menu_key_name(static_cast<int>(g_settings.window.open_key))));
	}

	void gui::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYDOWN && wparam == VK_ESCAPE)
		{
			if (g_input_service.is_open()) g_input_service.hide();
		}
	}

	void gui::update_banner()
	{
		if (m_banners.empty())
		{
			m_header = 0;
			m_header_size = { 0, 0 };
			return;
		}

		int active_index = g_settings.window.banner;
		if (active_index < 0 || static_cast<std::size_t>(active_index) >= m_banners.size())
			active_index = 0;

		m_header = m_banners[active_index]->id;
		m_header_size = m_banners[active_index]->dimensions;
	}

	bool gui::load_banner(const std::string& name, const unsigned char* bytes, int size)
	{
		int width = 0, height = 0;
		auto pixels = stbi_load_from_memory(bytes, size, &width, &height, nullptr, 4);
		if (!pixels)
		{
			LOG(WARNING) << "Cannot decode menu banner: " << name;
			return false;
		}

		auto id = g_renderer->upload_rgba(pixels, width, height);
		stbi_image_free(pixels);

		if (!id)
		{
			LOG(WARNING) << "Cannot upload menu banner: " << name;
			return false;
		}

		auto banner = std::make_unique<banner_item>();
		banner->name = name;
		banner->id = id;
		banner->dimensions = { width, height };

		m_banner_names.push_back(banner->name.c_str());
		m_banners.push_back(std::move(banner));
		return true;
	}

	void gui::load_textures()
	{
		m_banners.clear();
		m_banner_names.clear();

		for (const auto& banner : astra::banners())
			load_banner(std::string(banner.name), banner.image.data(), static_cast<int>(banner.image.size()));

		update_banner();
	}
}
