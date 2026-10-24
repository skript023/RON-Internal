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

		bool m_init{};
		// Sets up on the first Present: DX11 directly, DX12 through D3D11On12 on the game's direct queue.
		bool init(IDXGISwapChain* swapchain);
		void imgui_init();
		void on_present();
		void pre_reset();
		void post_reset();
		// Collects the game's direct queues; D3D11On12 must submit on the one its swapchain presents from.
		void capture_queue(ID3D12CommandQueue* queue);
		bool owns_swapchain(IDXGISwapChain* swapchain) const { return m_dxgi_swapchain.Get() == swapchain; }
		// Present and ResizeBuffers come from the render thread, wndproc from the game thread.
		std::recursive_mutex m_mutex;
		void merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas = false);
		ImTextureID upload_rgba(const unsigned char* pixels, int width, int height);
		void process_hwnd();
		void wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		ImFont* m_font{};
		ImFont* m_ui_manager_font{};
		ImFont* m_monospace_font{};
		HWND m_window{};

	private:
		bool create_targets();
		void release_targets();
		void load_texture(ID3D11Device* device);
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
