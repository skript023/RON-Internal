#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
	HRESULT APIENTRY hooks::swapchain_present(IDXGISwapChain* this_, UINT sync_interval, UINT flags)
	{
		TRY_CLAUSE
		{
			if (g_running)
			{
#ifdef UNIVERSAL
				if (g_renderer->m_init || g_renderer->init(this_))
					g_renderer->on_present();
#else
				g_renderer->on_present();
#endif
			}

#ifdef UNIVERSAL
			return detour_base::get_original<swapchain_present>()(this_, sync_interval, flags);
#else
			return g_hooking->m_swapchain_hook.get_original<decltype(&swapchain_present)>(swapchain_present_index)(this_, sync_interval, flags);
#endif
		} EXCEPT_CLAUSE

			return NULL;
	}


	HRESULT APIENTRY hooks::swapchain_resizebuffers(IDXGISwapChain* this_, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags)
	{
		TRY_CLAUSE
		{
			if (g_running)
			{
				g_renderer->pre_reset();

#ifdef UNIVERSAL
				auto result = detour_base::get_original<swapchain_resizebuffers>()(this_, buffer_count, width, height, new_format, swapchain_flags);
#else
				auto result = g_hooking->m_swapchain_hook.get_original<decltype(&swapchain_resizebuffers)>(swapchain_resizebuffers_index)
					(this_, buffer_count, width, height, new_format, swapchain_flags);
#endif
				if (SUCCEEDED(result))
				{
					g_renderer->post_reset();
				}

				return result;
			}

#ifdef UNIVERSAL
			return detour_base::get_original<swapchain_resizebuffers>()(this_, buffer_count, width, height, new_format, swapchain_flags);
#else
			return g_hooking->m_swapchain_hook.get_original<decltype(&swapchain_resizebuffers)>(swapchain_resizebuffers_index)
				(this_, buffer_count, width, height, new_format, swapchain_flags);
#endif
		} EXCEPT_CLAUSE

			return NULL;
	}

	//HRESULT hooks::swapchain_execute_command_list(ID3D12CommandQueue* queue, UINT num_command_list, ID3D12CommandList* command_list)
	//{
	//	if (!g_renderer->m_command_queue)
	//	{
	//		g_renderer->m_command_queue = queue;
	//	}

	//	// LOG(INFO) << "Queue Address: " << queue;

	//	return detour_base::get_original<swapchain_execute_command_list>()(queue, num_command_list, command_list);
	//}

	void APIENTRY hooks::swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
	{
		//return g_hooking->m_swapchain_draw_indexed_hook.get_original<decltype(&swapchain_draw_indexed)>()(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
	}
}