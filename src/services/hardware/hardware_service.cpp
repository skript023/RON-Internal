#include "hardware_service.hpp"
#include "data/gpu.hpp"

#include <comdef.h>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace big
{
	void hardware_service::init()
	{
		// Inisialisasi COM untuk WMI
		HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres)) {
			LOG(INFO) << "Failed to initialize COM library.";

			throw std::runtime_error("Failed to initialize COM library.");
		}

		// Set up security untuk WMI
		hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if (FAILED(hres)) {
			LOG(INFO) << "Failed to initialize security.";
			CoUninitialize();

			throw std::runtime_error("Failed to initialize security.");
		}

		load_hardware_info();

		LOG(INFO) << os_info;

		LOG(INFO) << gpu_info.to_json().dump();

		LOG(INFO) << cpu_info.to_json().dump();

		LOG(INFO) << motherboard_info.to_json().dump();
	}
	void hardware_service::destroy()
	{
		CoUninitialize();
	}
	bool hardware_service::load_os_info()
	{
		OSVERSIONINFOEX osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if (GetVersionEx((OSVERSIONINFO*)&osvi))
		{
			os_info = "Windows " + std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion) + " Build " + std::to_string(osvi.dwBuildNumber) + " " + osvi.szCSDVersion;

			return true;
		}
		else
		{
			LOG(INFO) << "Failed to retrieve OS info";
			return false;
		}
	}
	bool hardware_service::load_cpu_info()
	{
		try
		{
			HRESULT hres;

			IWbemLocator* pLoc = NULL;
			hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to create IWbemLocator object.";
				return false;
			}

			IWbemServices* pSvc = NULL;
			hres = pLoc->ConnectServer(SysAllocString(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to connect to WMI.";
				pLoc->Release();
				return false;
			}

			hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to set proxy blanket.";
				pSvc->Release();
				pLoc->Release();
				return false;
			}

			BSTR queryLanguage = SysAllocString(L"WQL");
			BSTR query = SysAllocString(L"SELECT * FROM Win32_Processor");
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = pSvc->ExecQuery(queryLanguage, query, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to execute WMI query.";
				pSvc->Release();
				pLoc->Release();
				SysFreeString(queryLanguage);
				SysFreeString(query);
				return false;
			}

			IWbemClassObject* pClassObject;
			ULONG uReturn = 0;
			while (pEnumerator) {
				hres = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn);
				if (0 == uReturn)
					break;

				VARIANT vtProp;
				hres = pClassObject->Get(L"Manufacturer", 0, &vtProp, 0, 0);
				cpu_info.Manufacturer = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"Name", 0, &vtProp, 0, 0);
				cpu_info.Name = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
				cpu_info.NumberOfCores = std::to_string(vtProp.intVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
				cpu_info.MaxClockSpeed = std::to_string(vtProp.intVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"NumberOfLogicalProcessors", 0, &vtProp, 0, 0);
				cpu_info.NumberOfLogicalProcessors = std::to_string(vtProp.intVal);
				VariantClear(&vtProp);

				pClassObject->Release();
			}

			// Bebaskan BSTR
			SysFreeString(queryLanguage);
			SysFreeString(query);

			pSvc->Release();
			pLoc->Release();

			return true;
		}
		catch (std::exception const& e)
		{
			LOG(INFO) << "Command failed with error: " << e.what();

			return false;
		}
	}
	bool hardware_service::load_gpu_info()
	{
		try 
		{
			HRESULT hres;
			IWbemLocator* pLoc = NULL;
			hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to create IWbemLocator object.";
				return false;
			}

			IWbemServices* pSvc = NULL;
			hres = pLoc->ConnectServer(SysAllocString(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to connect to WMI.";
				pLoc->Release();
				return false;
			}

			hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to set proxy blanket.";
				pSvc->Release();
				pLoc->Release();
				return false;
			}

			BSTR queryLanguage = SysAllocString(L"WQL");
			BSTR query = SysAllocString(L"SELECT * FROM Win32_VideoController");
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = pSvc->ExecQuery(queryLanguage, query, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to execute WMI query.";
				pSvc->Release();
				pLoc->Release();
				SysFreeString(queryLanguage);
				SysFreeString(query);
				return false;
			}

			IWbemClassObject* pClassObject;
			ULONG uReturn = 0;
			while (pEnumerator) {
				hres = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn);
				if (0 == uReturn)
					break;

				VARIANT vtProp;
				hres = pClassObject->Get(L"Name", 0, &vtProp, 0, 0);
				gpu_info.Name = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
				uint64_t adapterRAM = vtProp.uintVal;

				if (adapterRAM > 0 && adapterRAM < 4ULL * 1024 * 1024 * 1024) 
				{
					gpu_info.AdapterRAM = std::to_string(adapterRAM / (1024 * 1024));
				}
				else 
				{
					gpu_info.AdapterRAM = "Unknown or Overflow";
				}
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"DriverVersion", 0, &vtProp, 0, 0);
				gpu_info.DriverVersion = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				pClassObject->Release();
			}

			SysFreeString(queryLanguage);
			SysFreeString(query);
			pSvc->Release();
			pLoc->Release();

			return true;
		}
		catch (std::exception const& e) 
		{
			LOG(INFO) << "Command failed with error: " << e.what();

			return false;
		}
	}
	bool hardware_service::load_motherboard_info()
	{
		try
		{
			HRESULT hres;

			IWbemLocator* pLoc = NULL;
			hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to create IWbemLocator object.";
				return false;
			}

			IWbemServices* pSvc = NULL;
			hres = pLoc->ConnectServer(SysAllocString(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to connect to WMI.";
				pLoc->Release();
				return false;
			}

			hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to set proxy blanket.";
				pSvc->Release();
				pLoc->Release();
				return false;
			}

			BSTR queryLanguage = SysAllocString(L"WQL");
			BSTR query = SysAllocString(L"SELECT * FROM Win32_BaseBoard");
			IEnumWbemClassObject* pEnumerator = NULL;
			hres = pSvc->ExecQuery(queryLanguage, query, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
			if (FAILED(hres)) {
				LOG(INFO) << "Failed to execute WMI query.";
				pSvc->Release();
				pLoc->Release();
				SysFreeString(queryLanguage);
				SysFreeString(query);
				return false;
			}

			IWbemClassObject* pClassObject;
			ULONG uReturn = 0;
			while (pEnumerator) {
				hres = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn);
				if (0 == uReturn)
					break;

				VARIANT vtProp;
				hres = pClassObject->Get(L"Manufacturer", 0, &vtProp, 0, 0);
				motherboard_info.Manufacturer = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"Name", 0, &vtProp, 0, 0);
				motherboard_info.Name = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"SerialNumber", 0, &vtProp, 0, 0);
				motherboard_info.SerialNumber = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"SKU", 0, &vtProp, 0, 0);
				motherboard_info.SKU = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				hres = pClassObject->Get(L"Product", 0, &vtProp, 0, 0);
				motherboard_info.Product = BSTRToString(vtProp.bstrVal);
				VariantClear(&vtProp);

				pClassObject->Release();
			}

			SysFreeString(queryLanguage);
			SysFreeString(query);

			pSvc->Release();
			pLoc->Release();

			return true;
		}
		catch (std::exception const& e)
		{
			LOG(INFO) << "Command failed with error: " << e.what();

			return false;
		}
	}
	std::string hardware_service::BSTRToString(BSTR bstr)
	{
		std::wstring wstr(bstr, SysStringLen(bstr));

		std::string str(wstr.begin(), wstr.end());

		return str;
	}
}