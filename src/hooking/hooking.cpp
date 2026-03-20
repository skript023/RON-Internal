#include "common.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "function_types.hpp"

#include <MinHook.h>
#include <graphic/graphic_manager.hpp>
#include "memory/module.hpp"
#include <ui/canvas.hpp>
#include <utility/unreal_engine_utility.hpp>

namespace big
{
#ifdef UNIVERSAL
	hooking::hooking()
	{
		detour_hook::add<hooks::swapchain_present>("SwapChainPresent", graphic_manager::get_method_table(hooks::swapchain_present_index));
		detour_hook::add<hooks::swapchain_resizebuffers>("SwapChainResizeBuffers", graphic_manager::get_method_table(hooks::swapchain_resizebuffers_index));
		
		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());

		detour_hook::add<hooks::process_event>("ProcessEvent", g_pointers->m_process_event);

		g_hooking = this;
	}
#else
	hooking::hooking() : m_swapchain_hook(*g_pointers->m_swapchain, hooks::swapchain_num_funcs)
	{
		m_swapchain_hook.hook(hooks::swapchain_present_index, hooks::swapchain_present);
		m_swapchain_hook.hook(hooks::swapchain_resizebuffers_index, hooks::swapchain_resizebuffers);

		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());

		detour_hook::add<hooks::process_event>("ProcessEvent", g_pointers->m_process_event);

		g_hooking = this;
	}
#endif

	hooking::~hooking()
	{
		if (m_enabled)
			disable();

		g_hooking = nullptr;
	}

	void hooking::enable()
	{
#ifndef UNIVERSAL
		m_swapchain_hook.enable();
#endif

		m_og_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&hooks::wndproc)));

		detour_base::enable_all();

		MH_ApplyQueued();

		m_enabled = true;
	}

	void hooking::disable()
	{
		m_enabled = false;

		detour_base::disable_all();

		SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_og_wndproc));

#ifndef UNIVERSAL
		m_swapchain_hook.disable();
#endif

		MH_ApplyQueued();

		for (auto it : detour_base::hooks())
		{
			delete it;
		}
	}

	BOOL hooks::set_cursor_pos(int x, int y)
	{
		if (canvas::is_opened())
			return true;

		return detour_base::get_original<set_cursor_pos>()(x, y);
	}

	void* hooks::convert_thread_to_fiber(void* param)
	{
		if (IsThreadAFiber())
		{
			return GetCurrentFiber();
		}

		return detour_base::get_original<hooks::convert_thread_to_fiber>()(param);
	}
}
