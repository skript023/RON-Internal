#pragma once
#include "task.hpp"

namespace big
{
	class server_event
	{
	public:
		static void registration();
		static task<void> run();
	};
}
