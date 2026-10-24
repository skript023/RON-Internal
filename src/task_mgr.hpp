#pragma once
#include "common.hpp"
#include "task.hpp"

#include <mutex>
#include <vector>

namespace big
{
	class task_mgr
	{
		static task_mgr& get_instance()
		{
			static task_mgr instance;
			return instance;
		}

		void add_impl(task<void> t);
		void add_impl(task<void> (*func)());
		void clear_impl();
		void tick_impl();

	public:
		task_mgr() = default;
		~task_mgr() = default;

		static void add(task<void> t)
		{
			get_instance().add_impl(std::move(t));
		}

		static void add(task<void> (*func)())
		{
			get_instance().add_impl(func);
		}

		static void clear()
		{
			get_instance().clear_impl();
		}

		static void tick()
		{
			get_instance().tick_impl();
		}

	private:
		std::mutex m_incoming_mutex;
		std::vector<task<void>> m_incoming_tasks;
		std::vector<task<void>> m_tasks;
	};

	// Type trait untuk mendeteksi apakah suatu tipe bisa di-co_await
	template<typename T, typename = void>
	struct is_awaitable : std::false_type
	{
	};
	template<typename T>
	struct is_awaitable<T, std::void_t<decltype(std::declval<T>().operator co_await())>> : std::true_type
	{
	};
	// Khusus untuk struct task<T> kita yang memiliki await_ready / await_suspend
	template<typename T>
	struct is_awaitable<task<T>> : std::true_type
	{
	};
	template<typename T>
	inline constexpr bool is_awaitable_v = is_awaitable<std::decay_t<T>>::value;

	/**
	 * @brief Menjalankan callable yang mengembalikan task/coroutine dari konteks non-coroutine
	 */
	template<typename Coro, typename... Args>
	void async_run(Coro&& coro, Args&&... args)
	{
		using RetType = std::invoke_result_t<std::decay_t<Coro>, std::decay_t<Args>...>;

		if constexpr (std::is_void_v<RetType>)
		{
			// Callable biasa non-coroutine (void), pindahkan 'fn' secara by-value ke coroutine frame runner di heap.
			auto runner = [](std::decay_t<Coro> fn, std::decay_t<Args>... a) -> task<void> {
				fn(std::forward<decltype(a)>(a)...);
				co_return;
			};
			task_mgr::add(runner(std::forward<Coro>(coro), std::forward<Args>(args)...));
		}
		else
		{
			// Callable yang mengembalikan task/awaitable:
			// Simpan 'fn' secara by-value di coroutine frame runner pada heap
			auto runner = [](std::decay_t<Coro> fn, std::decay_t<Args>... a) -> task<void> {
				auto frame = fn(std::forward<decltype(a)>(a)...);
				using FrameType = std::decay_t<decltype(frame)>;
				if constexpr (is_awaitable_v<FrameType>)
				{
					co_await frame;
				}
				co_return;
			};
			task_mgr::add(runner(std::forward<Coro>(coro), std::forward<Args>(args)...));
		}
	}

	/**
	 * @brief Mengubah callable coroutine menjadi std::function<void()> biasa
	 * Sangat berguna untuk callback tombol ImGui atau delegate non-coroutine
	 */
	template<typename Coro>
	std::function<void()> async_func(Coro&& coro)
	{
		return [c = std::forward<Coro>(coro)]() mutable {
			async_run(c);
		};
	}
}
