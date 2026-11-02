#pragma once
#include "common.hpp"
#include "class/image_dimension.hpp"

namespace big
{
	using dx_callback = std::function<void()>;
	using wndproc_callback = std::function<void(HWND, UINT, WPARAM, LPARAM)>;

	struct banner_item
	{
		std::string name;
		ImTextureID id{};
		ImageDimensions dimensions{};
	};

	class gui
	{
		friend class dx12_impl;
		friend class dx11_impl;
		friend class renderer;

		std::map<uint32_t, dx_callback> m_dx_callbacks;
		std::vector<wndproc_callback> m_wndproc_callbacks;
	public:
		void init();
		void dx_init();

		/**
		 * @brief Add a callback function to draw your ImGui content in
		 *
		 * @param callback Function
		 * @param priority The higher the priority the value the later it gets drawn on top
		 * @return true
		 * @return false
		 */
		bool add_dx_callback(dx_callback callback, uint32_t priority);
		/**
		 * @brief Add a callback function on wndproc
		 *
		 * @param callback Function
		 */
		void add_wndproc_callback(wndproc_callback&& callback);

		// Decodes Astra's built-in banners and uploads them; call once the renderer has its device.
		void load_textures();
		bool load_banner(const std::string& name, const unsigned char* bytes, int size);
		// Points the header at the banner canvas::config().banner chooses.
		void update_banner();

		void dx_on_opened();

		void script_init();

		void wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	public:
		std::vector<std::unique_ptr<banner_item>> m_banners;
		std::vector<const char*> m_banner_names;

		ImTextureID m_header{};
		ImageDimensions m_header_size = { 0, 0 };
	};

	inline gui g_gui;
}
