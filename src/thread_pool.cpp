#include "thread_pool.hpp"
#include "pointers.hpp"

namespace big
{
	void thread_pool::setup_unreal_tls_for_thread()
	{
		// keep as is
	}

	thread_pool::thread_pool(const std::size_t preallocated_thread_count) :
		m_accept_jobs(true),
		m_allocated_thread_count(preallocated_thread_count),
		m_busy_threads(0)
	{
		rescale_thread_pool();

		m_timer_thread = std::thread(&thread_pool::timer_loop, this);

		g_thread_pool = this;
	}

	thread_pool::~thread_pool()
	{
		destroy();
		g_thread_pool = nullptr;
	}

	void thread_pool::destroy()
	{
		{
			std::unique_lock lock(m_lock);
			m_accept_jobs = false;
		}

		m_data_condition.notify_all();
		m_timer_cv.notify_all();

		for (auto& t : m_thread_pool)
			if (t.joinable()) t.join();

		if (m_timer_thread.joinable())
			m_timer_thread.join();

		m_thread_pool.clear();
		m_job_stack.clear();
	}

	void thread_pool::rescale_thread_pool()
	{
		m_thread_pool.reserve(m_allocated_thread_count);
		m_job_stack.resize(m_allocated_thread_count);

		for (size_t i = m_thread_pool.size(); i < m_allocated_thread_count; i++)
		{
			m_thread_pool.emplace_back(&thread_pool::run, this, i);
		}
	}

	// =====================
	// NORMAL JOB
	// =====================
	void thread_pool::queue_job(std::function<void()> func, std::source_location location)
	{
		if (!func) return;

		static std::atomic<size_t> next{ 0 };
		auto idx = next++ % m_job_stack.size();

		{
			std::unique_lock lock(m_lock);
			m_job_stack[idx].push_front({ func, nullptr, location });
		}

		m_data_condition.notify_one();
	}

	// =====================
	// COROUTINE
	// =====================
	void thread_pool::queue_coro(std::coroutine_handle<> h)
	{
		if (!h || h.done()) return;

		static std::atomic<size_t> next{ 0 };
		auto idx = next++ % m_job_stack.size();

		{
			std::unique_lock lock(m_lock);
			m_job_stack[idx].push_front({ {}, h });
		}

		m_data_condition.notify_one();
	}

	// =====================
	// TIMER
	// =====================
	void thread_pool::schedule_after(std::chrono::milliseconds delay, std::coroutine_handle<> h)
	{
		if (!h || h.done()) return;

		auto tp = std::chrono::steady_clock::now() + delay;

		{
			std::lock_guard lock(m_timer_lock);
			m_timers.push({ tp, h });
		}

		m_timer_cv.notify_one();
	}

	void thread_pool::timer_loop()
	{
		while (m_accept_jobs)
		{
			std::unique_lock lock(m_timer_lock);

			if (m_timers.empty())
			{
				m_timer_cv.wait(lock);
				continue;
			}

			auto now = std::chrono::steady_clock::now();
			auto next = m_timers.top().time;

			if (now >= next)
			{
				auto t = m_timers.top();
				m_timers.pop();

				lock.unlock();

				queue_coro(t.coro);
			}
			else
			{
				m_timer_cv.wait_until(lock, next);
			}
		}
	}

	// =====================
	// WORKER
	// =====================
	void thread_pool::run(size_t index)
	{
		setup_unreal_tls_for_thread();

		for (;;)
		{
			thread_pool_job job;

			std::unique_lock lock(m_lock);

			m_data_condition.wait(lock, [this, index]() {
				return !m_job_stack[index].empty() || !m_accept_jobs;
				});

			if (!m_accept_jobs)
				break;

			if (!m_job_stack[index].empty())
			{
				job = m_job_stack[index].front();
				m_job_stack[index].pop_front();
			}

			lock.unlock();

			++m_busy_threads;

			try
			{
				if (job.m_coro)
				{
					if (!job.m_coro.done())
						job.m_coro.resume();
				}
				else if (job.m_func)
				{
					std::invoke(job.m_func);
				}
			}
			catch (...)
			{
			}

			--m_busy_threads;
		}
	}
}