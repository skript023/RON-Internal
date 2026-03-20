#include "hooking.hpp"
#include "utility/joaat.hpp"
#include "process_event.hpp"
#include "commands/commands.hpp"

namespace big
{
	process_event::process_event(std::string name) :
		m_name(name),
		m_hash(joaat(name))
	{
		commands::add_process_event_command(this);
	}

	eExecutionStatus process_event::call(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		return on_call(_this, function, parms);
	}

	eExecutionStatus process_event::intercept(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		detour_base::get_original<hooks::process_event>()(_this, function, parms);
		//g_hooking->m_process_event_hook.get_original<decltype(&hooks::process_event)>(SDK::Offsets::ProcessEventIdx)(_this, function, parms);

		return eExecutionStatus::EXECUTION_INTERCEPTED;
	}

	void process_event::mark_dirty()
	{
		commands::mark_dirty();
	}
}