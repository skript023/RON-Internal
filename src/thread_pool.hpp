#pragma once
#include <coroutine>
#include "class/mutex.hpp"
#include "class/conditional_variable.hpp"

namespace big
{
	constexpr auto MAX_POOL_SIZE = 32u;

	struct thread_pool_job
	{
		std::function<void()> m_func{};
		std::coroutine_handle<> m_coro{ nullptr };
		std::source_location m_source_location{};
	};

	struct timer_task
	{
		std::chrono::steady_clock::time_point time;
		std::coroutine_handle<> coro;

		bool operator>(const timer_task& other) const
		{
			return time > other.time;
		}
	};

	class thread_pool
	{
		std::atomic<bool> m_accept_jobs;

		std::condition_variable m_data_condition;
		std::vector<std::deque<thread_pool_job>> m_job_stack;
		std::mutex m_lock;
		std::vector<std::thread> m_thread_pool;

		std::atomic<size_t> m_allocated_thread_count;
		std::atomic<size_t> m_busy_threads;

		// TIMER
		std::priority_queue<
			timer_task,
			std::vector<timer_task>,
			std::greater<>
		> m_timers;

		std::mutex m_timer_lock;
		std::condition_variable m_timer_cv;
		std::thread m_timer_thread;

	public:
		static void setup_unreal_tls_for_thread();
		thread_pool(const std::size_t preallocated_thread_count = 2);
		~thread_pool();

		void destroy();

		// NORMAL JOB
		void queue_job(std::function<void()> func,
			std::source_location location = std::source_location::current());

		// COROUTINE
		void queue_coro(std::coroutine_handle<> h);

		// TIMER
		void schedule_after(std::chrono::milliseconds delay, std::coroutine_handle<> h);

		// AWAITABLE
		struct schedule_awaitable
		{
			thread_pool& pool;

			bool await_ready() noexcept { return false; }

			void await_suspend(std::coroutine_handle<> h)
			{
				pool.queue_coro(h);
			}

			void await_resume() noexcept {}
		};

		struct sleep_awaitable
		{
			thread_pool& pool;
			std::chrono::milliseconds delay;

			bool await_ready() const noexcept
			{
				return delay.count() <= 0;
			}

			void await_suspend(std::coroutine_handle<> h)
			{
				pool.schedule_after(delay, h);
			}

			void await_resume() noexcept {}
		};

		auto schedule()
		{
			return schedule_awaitable{ *this };
		}

		auto sleep_for(std::chrono::milliseconds d)
		{
			return sleep_awaitable{ *this, d };
		}

		std::pair<size_t, size_t> usage() const
		{
			return { m_busy_threads, m_allocated_thread_count };
		}

	private:
		void run(size_t index);
		void rescale_thread_pool();
		void timer_loop();
	};

	inline thread_pool* g_thread_pool{};
}
