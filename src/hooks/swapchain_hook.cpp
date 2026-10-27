#include "hooking.hpp"
#include "renderer.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
    namespace
    {
        void render_present(IDXGISwapChain* swapchain, UINT flags)
        {
            if (!g_running || !g_renderer || (flags & DXGI_PRESENT_TEST)) return;
            std::lock_guard lock(g_renderer->m_mutex);
            if (g_renderer->init(swapchain))
                g_renderer->on_present();
        }

        template <auto Hook>
        HRESULT resize(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
        {
            auto original = detour_base::get_original<Hook>();
            if (!g_running || !g_renderer)
                return original(swapchain, count, width, height, format, flags);
            std::lock_guard lock(g_renderer->m_mutex);
            const bool active = g_renderer->m_init && g_renderer->owns_swapchain(swapchain);
            if (active) g_renderer->pre_reset();
            const auto result = original(swapchain, count, width, height, format, flags);
            // Reacquire the old buffers even when ResizeBuffers fails.
            if (active) g_renderer->post_reset();
            return result;
        }
    }

    HRESULT APIENTRY hooks::swapchain_present(IDXGISwapChain* swapchain, UINT interval, UINT flags)
    {
        // Always forward the game's Present, including after an overlay exception.
        TRY_CLAUSE
        {
            render_present(swapchain, flags);
        } EXCEPT_CLAUSE
        return detour_base::get_original<swapchain_present>()(swapchain, interval, flags);
    }

    HRESULT APIENTRY hooks::swapchain_present12(IDXGISwapChain* swapchain, UINT interval, UINT flags)
    {
        TRY_CLAUSE
        {
            render_present(swapchain, flags);
        } EXCEPT_CLAUSE
        return detour_base::get_original<swapchain_present12>()(swapchain, interval, flags);
    }

    HRESULT APIENTRY hooks::swapchain_resizebuffers(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        return resize<swapchain_resizebuffers>(swapchain, count, width, height, format, flags);
    }

    HRESULT APIENTRY hooks::swapchain_resizebuffers12(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        return resize<swapchain_resizebuffers12>(swapchain, count, width, height, format, flags);
    }

    void STDMETHODCALLTYPE hooks::execute_command_lists(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList* const* lists)
    {
        if (g_running && g_renderer) g_renderer->capture_queue(queue);
        detour_base::get_original<execute_command_lists>()(queue, count, lists);
    }

	void APIENTRY hooks::swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
	{
		//return g_hooking->m_swapchain_draw_indexed_hook.get_original<decltype(&swapchain_draw_indexed)>()(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
	}
}
