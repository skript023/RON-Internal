#include "hooking.hpp"
#include "process_event/process_event_service.hpp"

namespace big
{
	void APIENTRY hooks::process_event(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		// The trainer's handler and the game's function are guarded apart. A faulting handler still lets the game's
		// function run, but a game function that faulted is not run a second time, since part of it already ran.
		bool run_original = true;
		TRY_CLAUSE
		{
			run_original = process_event_service::interceptor(_this, function, parms);
		} EXCEPT_CLAUSE

		if (!run_original)
			return;

		TRY_CLAUSE
		{
			detour_base::get_original<hooks::process_event>()(_this, function, parms);
		} EXCEPT_CLAUSE
	}
}