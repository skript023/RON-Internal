#include "script.hpp"
#include "server_event.hpp"
#include "server/server_module.hpp"

namespace big
{
	void server_event::registration()
	{
		g_server_module->run();
	}
	void server_event::run()
	{
		LOG(INFO) << "Server Event Registered";

		while (g_running)
		{
			registration();
			script::get_current()->yield(5s);
		}
	}
}