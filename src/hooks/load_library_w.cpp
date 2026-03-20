#include "hooking.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	std::array<LPCWSTR, 13> block_dlls = {
		L"ACE-Base64.dll",
		L"ACE-SSC64.dll",
		L"ACE-DRV64.dll",
		L"ACE-DFS64.dll",
		L"ACE-CSI64.dll",
		L"ACE-ATS64.dll",
		L"ACE-IDS64.dll",
		L"ACE-Trace.dll",
		L"ACE-Safe.dll",
		L"ACE-Tips64.dll",
		L"SGuardAgent64.dll",
		L"CrashSight64.dll",
		L"PerfSight.dll",
	};

	HMODULE APIENTRY hooks::load_library_w(LPCWSTR libFileName)
	{
		char str[50];

		if (!g_settings.system.disable_anti_cheat)
			return detour_base::get_original<load_library_w>()(libFileName);

		for (LPCWSTR dll : block_dlls)
		{
			if (wcsstr(libFileName, dll) != 0)
			{
				auto converted = wcstombs(str, dll, sizeof(str));

				if (converted != (std::size_t)-1)
				{
					LOG(INFO) << "Blocked dll: " << str;
					g_notification_service.success("Anti-Cheat", "Anti-Cheat Blocked");
				}
				else
				{
					LOG(INFO) << "Anti-Cheat Blocked";
					g_notification_service.success("Anti-Cheat", "Anti-Cheat Blocked");
				}
				
				return nullptr;
			}
		}

		return detour_base::get_original<load_library_w>()(libFileName);
	}
}