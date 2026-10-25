#include "hooking.hpp"
#include "process_event/process_event_service.hpp"

namespace big
{
	void APIENTRY hooks::process_event(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		// The interceptor runs the game's function itself, so after a fault it is not run again: part of it may already
		// have run.
		TRY_CLAUSE
		{
			return process_event_service::interceptor(_this, function, parms);
		} EXCEPT_CLAUSE
	}
}