#include "main_event.hpp"
#include "script.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"
#include "utility/player.hpp"
#include "utility/game_settings.hpp"
#include "server/server_module.hpp"

namespace big
{
	void main_event::registration()
	{
		
	}
	void main_event::run()
	{
		LOG(INFO) << "Main Event Registered";
		commands::enable_bool_commands();

		while (g_running)
		{
			commands::run_looped_command();
			script::get_current()->yield();
		}
	}
}