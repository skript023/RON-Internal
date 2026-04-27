#pragma once
#include <coroutine>
#include <exception>
#include "thread_pool.hpp"

template<typename T = void>
struct task;

template<typename T>
struct task
{
	struct promise_type
	{
		T value;
		std::exception_ptr eptr;
		std::coroutine_handle<> continuation;

		task get_return_object()
		{
			return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}

		std::suspend_always initial_suspend() { return {}; }

		auto final_suspend() noexcept
		{
			struct awaiter {
				bool await_ready() noexcept { return false; }
				template<typename P>
				std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h)
				{
					return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
				}
				void await_resume() noexcept {}
			};
			return awaiter{};
		}

		void return_value(T v) { value = std::move(v); }
		void unhandled_exception() { eptr = std::current_exception(); }
	};

	using handle = std::coroutine_handle<promise_type>;
	handle coro;

	task(handle h) : coro(h) {}
	task(task&& o) : coro(o.coro) { o.coro = nullptr; }
	~task() { if (coro) coro.destroy(); }

	bool await_ready() { return false; }

	void await_suspend(std::coroutine_handle<> h)
	{
		coro.promise().continuation = h;
		big::g_thread_pool->queue_coro(coro);
	}

	T await_resume()
	{
		if (coro.promise().eptr)
			std::rethrow_exception(coro.promise().eptr);
		return coro.promise().value;
	}
};