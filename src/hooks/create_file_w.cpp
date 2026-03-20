#include "hooking.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	HANDLE WINAPI hooks::create_file_w(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		if (memcmp(lpFileName, L"\\\\.\\ACE-BASE", 12) == 0)
		{
			g_notification_service.success("Anti-Cheat", "Blocking Anti-Cheat Communication With Kernel Driver");

			SuspendThread(GetCurrentThread());
			LOG(INFO) << "Blocking Anti-Cheat Communication With Kernel Driver";

			return nullptr;
		}

		return detour_base::get_original<create_file_w>()(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
}