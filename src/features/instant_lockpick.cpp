#include "commands/bool_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	static bool_command _instant_lockpick("instant_lockpick", "Instant Lockpick", "Lockpicking with the multitool, and its other jobs, finish almost at once", false);

	// While the multitool works on something, such as a door's lock, it keeps asking that thing how long the job takes;
	// a short answer finishes it. Not zero, in case the time is divided by.
	class multitool_use_time : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			if (!_instant_lockpick.get_state())
				return eExecutionStatus::EXECUTION_CONTINUE;

			static_cast<SDK::Params::CanUseMultitoolOn_GetMultitoolUseTime*>(parms)->ReturnValue = 0.05f;
			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static multitool_use_time _multitool_use_time("Function ReadyOrNot.CanUseMultitoolOn.GetMultitoolUseTime");
}
