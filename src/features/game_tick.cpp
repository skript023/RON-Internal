#include "process_event/process_event.hpp"
#include "task_mgr.hpp"
#include "renderer.hpp"

#include "SDK/UMG_parameters.hpp"

namespace big::features
{
	class script_tick : public process_event
	{
		using process_event::process_event;

		virtual eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms)
		{
			// ReceiveTick runs on the game thread for every ticking component, many times a frame. Tasks run once
			// per presented frame, or every 50 ms while nothing is presented, such as before the swapchain is hooked.
			static std::uint64_t last_frame = 0;
			static auto last_tick = std::chrono::steady_clock::now();

			const auto frame = g_present_count.load(std::memory_order_relaxed);
			const auto now = std::chrono::steady_clock::now();
			if (frame == last_frame && now - last_tick < 50ms)
				return eExecutionStatus::EXECUTION_CONTINUE;

			last_frame = frame;
			last_tick = now;

			task_mgr::tick();

			static bool running = false;

			if (!running)
			{
				LOG(INFO) << "Tick Running";

				running = true;
			}

			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	static script_tick _script_tick("Function Engine.ActorComponent.ReceiveTick");

	// Engine.ActorComponent.ReceiveTick came only from a few components, such as one on the M4 SR, so holding another
	// gun stopped every task. The player controller and the player's character override ReceiveTick in their own
	// Blueprints and tick every frame.
	static script_tick _controller_tick("Function BP_BaseController.BP_BaseController_C.ReceiveTick");
	static script_tick _player_tick("Function BasePlayer.BasePlayer_C.ReceiveTick");
}
