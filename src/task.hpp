#pragma once
#include "common.hpp"

#include <coroutine>
#include <chrono>
#include <optional>
#include <utility>

namespace big
{
	// 1. Awaiter untuk Jeda Waktu (Yield)
	struct [[nodiscard]] task_yield
	{
		std::optional<std::chrono::high_resolution_clock::duration> m_time;
		explicit task_yield(std::optional<std::chrono::high_resolution_clock::duration> t = std::nullopt) :
		    m_time(t)
		{
		}
	};

	// Base promise untuk mendukung relasi hirarki parent <-> child
	struct task_promise_base
	{
		task_promise_base* m_parent{nullptr};
		task_promise_base* m_child{nullptr};
		std::optional<std::chrono::high_resolution_clock::time_point> m_wake_time{};

		virtual ~task_promise_base() = default;
		virtual std::coroutine_handle<> get_handle() = 0;

		task_promise_base* get_leaf()
		{
			task_promise_base* cur = this;
			while (cur->m_child != nullptr)
			{
				cur = cur->m_child;
			}
			return cur;
		}
	};

	// 2. Deklarasi Awal Template
	template<typename T = void>
	struct [[nodiscard]] task;

	// Final awaiter: saat subtask selesai, transfer kembali kontrol ke parent
	template<typename PromiseType>
	struct task_final_awaiter
	{
		bool await_ready() noexcept
		{
			return false;
		}

		std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> h) noexcept
		{
			auto* parent = h.promise().m_parent;
			if (parent)
			{
				parent->m_child = nullptr;
				return parent->get_handle();
			}
			return std::noop_coroutine();
		}

		void await_resume() noexcept
		{
		}
	};

	// 3. Spesialisasi untuk task<void>
	template<>
	struct [[nodiscard]] task<void>
	{
		struct promise_type : task_promise_base
		{
			task get_return_object()
			{
				return task{std::coroutine_handle<promise_type>::from_promise(*this)};
			}

			std::suspend_always initial_suspend() noexcept
			{
				return {};
			}

			auto final_suspend() noexcept
			{
				return task_final_awaiter<promise_type>{};
			}

			void return_void() noexcept
			{
			}

			void unhandled_exception()
			{
				try
				{
					std::rethrow_exception(std::current_exception());
				}
				catch (const std::exception& e)
				{
					LOG(WARNING) << "Task exception: " << e.what();
				}
				catch (...)
				{
					LOG(WARNING) << "Task unknown exception";
				}
			}

			std::coroutine_handle<> get_handle() override
			{
				return std::coroutine_handle<promise_type>::from_promise(*this);
			}

			// Support untuk co_await task_yield(...)
			auto await_transform(task_yield yield_info)
			{
				struct awaiter
				{
					task_yield info;
					bool await_ready() noexcept
					{
						return false;
					}

					void await_suspend(std::coroutine_handle<promise_type> h) noexcept
					{
						if (info.m_time.has_value())
							h.promise().m_wake_time = std::chrono::high_resolution_clock::now() + info.m_time.value();
						else
							h.promise().m_wake_time = std::nullopt;
					}

					void await_resume() noexcept
					{
					}
				};
				return awaiter{yield_info};
			}

			// Support untuk co_await awaitable lainnya (subtask, etc.)
			template<typename Awaitable>
			    requires(!std::is_same_v<std::decay_t<Awaitable>, task_yield>)
			decltype(auto) await_transform(Awaitable&& awaitable)
			{
				return std::forward<Awaitable>(awaitable);
			}
		};

		std::coroutine_handle<promise_type> m_handle{nullptr};

		explicit task(std::coroutine_handle<promise_type> h) noexcept : m_handle(h) {}
		task() noexcept = default;
		~task()
		{
			if (m_handle)
			{
				m_handle.destroy();
				m_handle = nullptr;
			}
		}

		task(task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
		task& operator=(task&& other) noexcept
		{
			if (this != &other)
			{
				if (m_handle)
					m_handle.destroy();
				m_handle = std::exchange(other.m_handle, nullptr);
			}
			return *this;
		}

		task(const task&) = delete;
		task& operator=(const task&) = delete;

		// --- FUNGSI AGAR BISA DI-AWAIT OLEH TASK LAIN ---
		bool await_ready() const noexcept
		{
			return !m_handle || m_handle.done();
		}

		template<typename CallerPromise>
		std::coroutine_handle<> await_suspend(std::coroutine_handle<CallerPromise> caller) noexcept
		{
			if constexpr (std::is_base_of_v<task_promise_base, CallerPromise>)
			{
				caller.promise().m_child = &m_handle.promise();
				m_handle.promise().m_parent = &caller.promise();
			}
			return m_handle;
		}

		void await_resume()
		{
			if (m_handle)
			{
				m_handle.destroy();
				m_handle = nullptr;
			}
		}
	};

	// 4. Implementasi Generic untuk task<T> (Dengan Return Value)
	template<typename T>
	struct [[nodiscard]] task
	{
		struct promise_type : task_promise_base
		{
			std::optional<T> m_value;

			task get_return_object()
			{
				return task{std::coroutine_handle<promise_type>::from_promise(*this)};
			}

			std::suspend_always initial_suspend() noexcept
			{
				return {};
			}

			auto final_suspend() noexcept
			{
				return task_final_awaiter<promise_type>{};
			}

			void return_value(T val)
			{
				m_value.emplace(std::move(val));
			}

			void unhandled_exception()
			{
				try
				{
					std::rethrow_exception(std::current_exception());
				}
				catch (const std::exception& e)
				{
					LOG(WARNING) << "Task exception: " << e.what();
				}
				catch (...)
				{
					LOG(WARNING) << "Task unknown exception";
				}
			}

			std::coroutine_handle<> get_handle() override
			{
				return std::coroutine_handle<promise_type>::from_promise(*this);
			}

			auto await_transform(task_yield yield_info)
			{
				struct awaiter
				{
					task_yield info;
					bool await_ready() noexcept
					{
						return false;
					}

					void await_suspend(std::coroutine_handle<promise_type> h) noexcept
					{
						if (info.m_time.has_value())
							h.promise().m_wake_time = std::chrono::high_resolution_clock::now() + info.m_time.value();
						else
							h.promise().m_wake_time = std::nullopt;
					}

					void await_resume() noexcept
					{
					}
				};
				return awaiter{yield_info};
			}

			template<typename Awaitable>
			    requires(!std::is_same_v<std::decay_t<Awaitable>, task_yield>)
			decltype(auto) await_transform(Awaitable&& awaitable)
			{
				return std::forward<Awaitable>(awaitable);
			}
		};

		std::coroutine_handle<promise_type> m_handle{nullptr};

		explicit task(std::coroutine_handle<promise_type> h) noexcept : m_handle(h) {}
		task() noexcept = default;
		~task()
		{
			if (m_handle)
			{
				m_handle.destroy();
				m_handle = nullptr;
			}
		}

		task(task&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
		task& operator=(task&& other) noexcept
		{
			if (this != &other)
			{
				if (m_handle)
					m_handle.destroy();
				m_handle = std::exchange(other.m_handle, nullptr);
			}
			return *this;
		}

		task(const task&) = delete;
		task& operator=(const task&) = delete;

		bool await_ready() const noexcept
		{
			return !m_handle || m_handle.done();
		}

		template<typename CallerPromise>
		std::coroutine_handle<> await_suspend(std::coroutine_handle<CallerPromise> caller) noexcept
		{
			if constexpr (std::is_base_of_v<task_promise_base, CallerPromise>)
			{
				caller.promise().m_child = &m_handle.promise();
				m_handle.promise().m_parent = &caller.promise();
			}
			return m_handle;
		}

		T await_resume()
		{
			T result = std::move(m_handle.promise().m_value.value());
			if (m_handle)
			{
				m_handle.destroy();
				m_handle = nullptr;
			}
			return result;
		}
	};

	namespace this_task
	{
		inline task_yield wait(std::optional<std::chrono::high_resolution_clock::duration> t = std::nullopt)
		{
			return task_yield(t);
		}
	}
}
