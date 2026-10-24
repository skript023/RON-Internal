#include "script.hpp"
#include "server_event.hpp"
#include "server/server_module.hpp"

namespace big
{
	void server_event::registration()
	{
		g_server_module->run();
	}
	task<void> server_event::run()
	{
		LOG(INFO) << "Server Event Registered";

		while (g_running)
		{
			registration();
			co_await this_task::wait(5s);
		}

		co_return;
	}
}
