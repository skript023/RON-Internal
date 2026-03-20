#include "process_event/process_event.hpp"
#include <script_mgr.hpp>

#include "SDK/UMG_parameters.hpp"

namespace big::features
{
	class script_tick : public process_event
	{
		using process_event::process_event;

		virtual eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms)
		{
			g_script_mgr.tick();

			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	static script_tick _script_tick("Function Engine.ActorComponent.ReceiveTick");
}