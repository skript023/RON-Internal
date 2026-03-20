#include "common.hpp"
#include "hooking.hpp"
#include "notification/notification_service.hpp"

namespace big
{
#ifndef ProcessDebugFlags
#define ProcessDebugFlags ((PROCESSINFOCLASS)31)
#endif

#ifndef ProcessDebugObjectHandle
#define ProcessDebugObjectHandle ((PROCESSINFOCLASS)30)
#endif

	NTSTATUS NTAPI hooks::nt_query_information_process(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
	{
		NTSTATUS status = detour_base::get_original<nt_query_information_process>()(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

		if (NT_SUCCESS(status)) 
		{
			switch (ProcessInformationClass) 
			{
			case ProcessDebugPort: // 7
				*(PHANDLE)ProcessInformation = NULL;
				g_notification_service.success("Anti-Tamper", "Blocking Anti Tamper Debug");
				break;
			case ProcessDebugFlags: // 31
				*(PULONG)ProcessInformation = 1;
				g_notification_service.success("Anti-Tamper", "Blocking Anti Tamper Debug");
				break;
			case ProcessDebugObjectHandle: // 30
				*(PHANDLE)ProcessInformation = NULL;
				g_notification_service.success("Anti-Tamper", "Blocking Anti Tamper Debug");
				break;
			}
		}

		return status;
	}
}