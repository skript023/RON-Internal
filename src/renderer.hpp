#pragma once
#include "common.hpp"
#include <imgui.h>

namespace big
{
	class renderer
	{
	public:
		explicit renderer();
		~renderer();

		bool m_init = false;
#ifdef UNIVERSAL
		bool init(IDXGISwapChain* swapchain);
#else
		bool init();
#endif // UNIVERSAL

		void imgui_init();

		void on_present();

		void pre_reset();
		void post_reset();
		void merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas = false);

		void load_texture(ID3D11Device* resource);
		void process_hwnd();
		void wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	public:
		ImFont* m_font = nullptr;
		ImFont* m_ui_manager_font = nullptr;
		ImFont* m_monospace_font = nullptr;

		HWND m_window = NULL;
	private:
		comptr<IDXGISwapChain> m_dxgi_swapchain;
		comptr<ID3D11Device> m_d3d_device;
		comptr<ID3D11DeviceContext> m_d3d_context;
		ID3D11RenderTargetView* m_d3d_render_target = nullptr;
	};

	inline renderer* g_renderer{};
}
