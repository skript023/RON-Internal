#pragma once
#include "common.hpp"
#include <imgui.h>
#include <d3d11on12.h>

#define UNIVERSAL

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
        void capture_queue(ID3D12CommandQueue* queue);
        bool owns_swapchain(IDXGISwapChain* swapchain) const { return m_dxgi_swapchain.Get() == swapchain; }
        std::recursive_mutex m_mutex;
		void merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas = false);

		void load_texture(ID3D11Device* resource);
		ImTextureID upload_rgba(const unsigned char* pixels, int width, int height);
		void process_hwnd();
		void wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	public:
		ImFont* m_font = nullptr;
		ImFont* m_ui_manager_font = nullptr;
		ImFont* m_monospace_font = nullptr;

		HWND m_window = NULL;
	private:
        bool create_targets();
        void release_targets();
        comptr<ID3D11On12Device> m_on12;
        comptr<IDXGISwapChain3> m_swapchain3;
        comptr<ID3D12CommandQueue> m_queue;
        std::mutex m_queue_mutex;
        std::vector<comptr<ID3D12CommandQueue>> m_queues;
        std::vector<comptr<ID3D11Resource>> m_wrapped_buffers;
        std::vector<comptr<ID3D11RenderTargetView>> m_targets;
        comptr<IDXGISwapChain> m_dxgi_swapchain;
		comptr<ID3D11Device> m_d3d_device;
		comptr<ID3D11DeviceContext> m_d3d_context;
		std::vector<comptr<ID3D11ShaderResourceView>> m_textures;
	};

	inline renderer* g_renderer{};
	inline std::atomic<std::uint64_t> g_present_count{0};
}
