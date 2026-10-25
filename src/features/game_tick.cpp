#include "game_tick.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "task_mgr.hpp"

namespace big::game_tick
{
	// The game thread owns the game window. Animation and widget events can also run on worker threads, which must
	// never resume a task.
	static DWORD game_thread_id()
	{
		static std::atomic<DWORD> id{0};
		if (!id && g_pointers && g_pointers->m_hwnd)
			id = GetWindowThreadProcessId(g_pointers->m_hwnd, nullptr);
		return id;
	}

	// One kind of event alone was not enough: Engine.ActorComponent.ReceiveTick came only from a few components, such
	// as one on the M4 SR, so holding another gun stopped every task. Characters' animation, the HUD's widgets and the
	// AI's behaviour trees raise these events every frame in a mission.
	void on_frame_event()
	{
		if (GetCurrentThreadId() != game_thread_id())
			return;

		// A task that calls into the game can raise another of these events; that one waits for the next frame.
		static bool ticking = false;
		if (ticking)
			return;

		static std::uint64_t last_frame = 0;
		static auto last_tick = std::chrono::steady_clock::now();
		static std::uint32_t calls = 0;

		const auto frame = g_present_count.load(std::memory_order_relaxed);
		if (frame == last_frame)
		{
			// Hundreds of these events arrive in a frame, so the clock is read only on every 64th.
			if (++calls % 64 != 0 || std::chrono::steady_clock::now() - last_tick < 50ms)
				return;
		}

		last_frame = frame;
		last_tick = std::chrono::steady_clock::now();

		ticking = true;
		task_mgr::tick();
		ticking = false;

		static bool running = false;
		if (!running)
		{
			LOG(INFO) << "Tick Running";
			running = true;
		}
	}
}
