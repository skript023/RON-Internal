#include "common.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "function_types.hpp"

#include <graphic/graphic_manager.hpp>
#include "memory/module.hpp"
#include <astra/host/canvas.hpp>
#include <utility/unreal_engine_utility.hpp>

#include <ellohim/logger.hpp>

namespace big
{
	// Ellohim-Hook reports what it patched, and why a patch was refused, through the trainer's own log.
	static void route_ellohim_log()
	{
		ellohim::logger::set_callback([](ellohim::log_level level, std::string_view message) {
			if (level == ellohim::log_level::info)
				LOG(INFO) << "[Ellohim] " << message;
			else
				LOG(WARNING) << "[Ellohim] " << message;
		});
	}

	// A hook that could not be created leaves none of the others behind.
	static void delete_all_hooks()
	{
		while (!detour_base::hooks().empty())
			delete detour_base::hooks().back();
	}

#ifdef UNIVERSAL
	hooking::hooking()
	try
	{
		route_ellohim_log();

		// Ready or Not runs DX12 by default and DX11 on request. A DX12 dummy gives ExecuteCommandLists, where the
		// overlay finds the game's direct queue; its Present is hooked separately only where dxgi.dll gives it its own.
		void* present12 = nullptr;
		void* resize12 = nullptr;
		if (graphic_manager::get_swapchain(graphics_api::directx12) == eInitializationStatus::SUCCESS)
		{
			present12 = graphic_manager::get_method_table(132 + hooks::swapchain_present_index);
			resize12 = graphic_manager::get_method_table(132 + hooks::swapchain_resizebuffers_index);
			detour_hook::add<hooks::execute_command_lists>("ExecuteCommandLists", graphic_manager::get_method_table(44 + 10));
		}
		void* present11 = nullptr;
		void* resize11 = nullptr;
		if (graphic_manager::get_swapchain(graphics_api::directx11) == eInitializationStatus::SUCCESS)
		{
			present11 = graphic_manager::get_method_table(hooks::swapchain_present_index);
			resize11 = graphic_manager::get_method_table(hooks::swapchain_resizebuffers_index);
			detour_hook::add<hooks::swapchain_present>("SwapChainPresent", present11);
			detour_hook::add<hooks::swapchain_resizebuffers>("SwapChainResizeBuffers", resize11);
		}
		if (present12 && present12 != present11)
			detour_hook::add<hooks::swapchain_present12>("SwapChainPresent12", present12);
		if (resize12 && resize12 != resize11)
			detour_hook::add<hooks::swapchain_resizebuffers12>("SwapChainResizeBuffers12", resize12);
		if (!present11 && !present12)
			throw std::runtime_error("No DX11/DX12 swapchain hooks available");

		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		//detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());

		detour_hook::add<hooks::process_event>("ProcessEvent", g_pointers->m_process_event);

		g_hooking = this;
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Hook initialization failed: " << e.what();
		delete_all_hooks();
		throw;
	}
#else
	hooking::hooking()
	try : m_swapchain_hook(*g_pointers->m_swapchain, hooks::swapchain_num_funcs)
	{
		route_ellohim_log();

		m_swapchain_hook.hook(hooks::swapchain_present_index, reinterpret_cast<void*>(&hooks::swapchain_present));
		m_swapchain_hook.hook(hooks::swapchain_resizebuffers_index, reinterpret_cast<void*>(&hooks::swapchain_resizebuffers));

		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		//detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());

		detour_hook::add<hooks::process_event>("ProcessEvent", g_pointers->m_process_event);

		g_hooking = this;
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Hook initialization failed: " << e.what();
		delete_all_hooks();
		throw;
	}
#endif

	hooking::~hooking()
	{
		if (m_enabled)
			disable();

		delete_disabled_hooks();

		g_hooking = nullptr;
	}

	void hooking::delete_disabled_hooks()
	{
		// A hook stays enabled when another program, such as the Steam or Discord overlay, hooked the same function on
		// top of it: that program still calls the detour, so the detour and its trampoline must stay alive.
		std::vector<detour_base*> disabled;
		for (auto* hook : detour_base::hooks())
		{
			if (!hook->is_enabled())
				disabled.push_back(hook);
		}
		for (auto* hook : disabled)
			delete hook;
	}

	void hooking::enable()
	{
#ifndef UNIVERSAL
		m_swapchain_hook.enable();
#endif

		m_og_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&hooks::wndproc)));

		// All or nothing: a hook that cannot be placed takes the others back off.
		if (!detour_base::enable_all())
		{
			SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_og_wndproc));
			detour_base::disable_all();
#ifndef UNIVERSAL
			m_swapchain_hook.disable();
#endif
			throw std::runtime_error("Could not enable all hooks");
		}

		m_enabled = true;
	}

	void hooking::disable()
	{
		m_enabled = false;

		SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_og_wndproc));

		detour_base::disable_all();

#ifndef UNIVERSAL
		m_swapchain_hook.disable();
#endif

		for (auto* hook : detour_base::hooks())
		{
			if (hook->is_enabled())
				LOG(WARNING) << "Hook '" << hook->name() << "' is covered by another program's hook; it stays in place.";
		}
		delete_disabled_hooks();
	}

	BOOL hooks::set_cursor_pos(int x, int y)
	{
		// Only while the open menu uses the mouse; a keyboard-only List leaves mouse look to the game.
		if (canvas::uses_mouse())
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
