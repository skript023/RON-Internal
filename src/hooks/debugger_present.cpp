#include "hooking.hpp"

namespace big
{
	BOOL APIENTRY hooks::is_debugger_present()
	{
		return FALSE;
	}
}