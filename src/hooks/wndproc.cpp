#include "hooking.hpp"
#include "renderer.hpp"
#include <astra/host/canvas.hpp>

namespace big
{
	LRESULT hooks::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (g_running) [[likely]]
		{
			g_renderer->wndproc(hwnd, msg, wparam, lparam);

			// While the cursor is on the menu or a menu field is typing, the game does not get that input.
			if (auto result = canvas::block_game_message(hwnd, msg, wparam, lparam))
				return *result;
		}

		return CallWindowProcW(g_hooking->m_og_wndproc, hwnd, msg, wparam, lparam);
	}
}
