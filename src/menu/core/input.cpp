#include "../view.hpp"
#include "pointers.hpp"
#include "task_mgr.hpp"

#include "imgui.h"
#include <astra/host/input_service.hpp>
#include "utility/unreal_engine_utility.hpp"

namespace big
{
	void view::draw_input()
	{
		// The player's input is switched on the game thread, once when the box opens and once when it closes.
		static bool was_open = false;
		const bool open = input_service::is_open();
		if (open != was_open)
		{
			was_open = open;
			async_run([open] {
				if (open)
					unreal_engine::disable_input();
				else
					unreal_engine::enable_input();
			});
		}

		// Astra draws the box itself, in the menu's theme, and holds the mouse until the box closes.
		input_service::draw();
	}
}
