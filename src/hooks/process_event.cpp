#include "hooking.hpp"
#include "process_event/process_event_service.hpp"

namespace big
{
	void APIENTRY hooks::process_event(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		TRY_CLAUSE
		{
			return process_event_service::interceptor(_this, function, parms);
		} EXCEPT_CLAUSE

			return detour_base::get_original<hooks::process_event>()(_this, function, parms);
		//return g_hooking->m_process_event.get_original<decltype(&process_event)>(SDK::Offsets::ProcessEventIdx)(_this, function, parms);
	}
}