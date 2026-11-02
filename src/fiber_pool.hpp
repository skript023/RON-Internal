#pragma once
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
