#pragma once
#include "task.hpp"

namespace big
{
	class main_event
	{
	public:
		static void registration();
		static task<void> run();
	};
}
