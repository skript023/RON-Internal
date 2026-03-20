#include "process_event_service.hpp"

#include "hooking.hpp"
#include "script_mgr.hpp"

#include "process_event.hpp"
#include <commands/commands.hpp>

#include "utility/unreal_engine_utility.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	void process_event_service::interceptor_impl(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		auto name = function->GetFullName();

		if (g_settings.debug.logging_process_event)
			LOG(INFO) << name;

		auto func = joaat(name);

		if (auto command = commands::get_process_event(func); command)
		{
			if (auto status = command->call(_this, function, parms); status == eExecutionStatus::EXECUTION_INTERCEPTED)
			{
				return;
			}
		}

		return detour_base::get_original<hooks::process_event>()(_this, function, parms);
		//return g_hooking->m_process_event_hook.get_original<decltype(&hooks::process_event)>(SDK::Offsets::ProcessEventIdx)(_this, function, parms);
	}
}