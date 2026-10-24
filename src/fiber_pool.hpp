#pragma once

// Compatibility header for code and libraries (astra-ui) that queue jobs through g_fiber_pool. Win32 fibers have
// been replaced by C++20 coroutines (task.hpp & task_mgr.hpp): a queued job runs on the game thread at the next tick.
#include "task.hpp"
#include "task_mgr.hpp"

namespace big
{
	class fiber_pool
	{
	public:
		template<typename F>
		void queue_job(F&& func)
		{
			async_run(std::forward<F>(func));
		}
	};

	inline fiber_pool g_fiber_pool_instance;
	inline fiber_pool* g_fiber_pool = &g_fiber_pool_instance;
}
