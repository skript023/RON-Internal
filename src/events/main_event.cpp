#include "main_event.hpp"
#include "script.hpp"
#include "pointers.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"

#include <astra/host/hotkeys.hpp>
#include <astra/host/settings.hpp>

namespace big
{
	static void update()
	{
		// Feature switches the Astra loader wrote to settings.json, then the per-feature hotkeys.
		settings::apply_external();
		hotkeys::tick(g_pointers->m_hwnd);

		commands::run_looped_command();
	}

	// A fault in one feature skips this tick instead of ending every looped feature for the session.
	static void update_guarded()
	{
		TRY_CLAUSE
		{
			update();
		}
		EXCEPT_CLAUSE
	}

	void main_event::registration()
	{

	}
	task<void> main_event::run()
	{
		LOG(INFO) << "Main Event Registered";
		commands::enable_bool_commands();

		while (g_running)
		{
			update_guarded();
			co_await this_task::wait();
		}

		co_return;
	}
}
