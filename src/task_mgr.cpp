#include "task_mgr.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
	// MSVC allows __try only in a function with nothing to unwind, so the resume that may fault lives on its own.
	static bool resume_guarded(std::coroutine_handle<> handle)
	{
		__try
		{
			handle.resume();
			return true;
		}
		__except (big::vectored_exception_handler(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	void task_mgr::add_impl(task<void> t)
	{
		std::scoped_lock lock(m_incoming_mutex);
		m_incoming_tasks.push_back(std::move(t));
	}

	void task_mgr::add_impl(task<void> (*func)())
	{
		if (func)
		{
			add_impl(func());
		}
	}

	void task_mgr::clear_impl()
	{
		{
			std::scoped_lock lock(m_incoming_mutex);
			m_incoming_tasks.clear();
		}
		m_tasks.clear();
	}

	void task_mgr::tick_impl()
	{
		// 1. Pindahkan semua task baru dari incoming queue ke active task list secara thread-safe
		{
			std::scoped_lock lock(m_incoming_mutex);
			if (!m_incoming_tasks.empty())
			{
				m_tasks.insert(m_tasks.end(),
				               std::make_move_iterator(m_incoming_tasks.begin()),
				               std::make_move_iterator(m_incoming_tasks.end()));
				m_incoming_tasks.clear();
			}
		}

		// 2. Jalankan semua task aktif (iterasi aman berdasarkan index tanpa mutasi buffer)
		const size_t task_count = m_tasks.size();
		for (size_t i = 0; i < task_count; ++i)
		{
			auto& handle = m_tasks[i].m_handle;

			if (!handle || handle.done())
				continue;

			auto* leaf = handle.promise().get_leaf();
			bool should_run = true;

			// Periksa status jeda/waktu tunggu di leaf Task
			if (leaf->m_wake_time.has_value())
			{
				if (std::chrono::high_resolution_clock::now() < leaf->m_wake_time.value())
				{
					should_run = false; // Belum waktunya jalan
				}
				else
				{
					leaf->m_wake_time = std::nullopt; // Reset wake time
				}
			}

			// Resume eksekusi coroutine pada leaf aktif
			// A task that faulted is dropped; its frame is left as it is, since unwinding it could fault again.
			if (should_run && !resume_guarded(leaf->get_handle()))
			{
				handle = nullptr;
			}
		}

		// 3. Bersihkan task yang sudah selesai setelah SEMUA task frame ini selesai di-tick
		std::erase_if(m_tasks, [](const task<void>& t) {
			return !t.m_handle || t.m_handle.done();
		});
	}
}
