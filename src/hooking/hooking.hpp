#pragma once
#include "MinHook.h"
#include "common.hpp"
#include "renderer.hpp"
#include "detour_hook.hpp"
#include "hooking/vmt_hook.hpp"
#include "hooking/swap_pointer_hook.hpp"
#include "SDK/Engine_classes.hpp"
#include <winternl.h>

namespace big
{
	struct hooks
	{
		static constexpr auto swapchain_num_funcs = 18;
		static constexpr auto swapchain_present_index = 8;
		static constexpr auto swapchain_resizebuffers_index = 13;
		static HRESULT APIENTRY swapchain_present(IDXGISwapChain* this_, UINT sync_interval, UINT flags);
		static HRESULT APIENTRY swapchain_resizebuffers(IDXGISwapChain* this_, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);
		static void APIENTRY swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

		static LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);
		static void* convert_thread_to_fiber(void* param);
		static HMODULE APIENTRY load_library_w(LPCWSTR libFileName);
		static BOOL APIENTRY is_debugger_present();
		static NTSTATUS NTAPI nt_query_information_process(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
		static NTSTATUS NTSYSCALLAPI  nt_query_virtual_memory(void* _this, HANDLE handle, PVOID base_addr, int info_class, MEMORY_BASIC_INFORMATION* info, int size, size_t* return_len);
		static HANDLE WINAPI create_file_w(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
		static BOOL WINAPI write_file(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
		
		static void APIENTRY process_event(SDK::UObject* _this, SDK::UFunction* function, void* parms);
		static void* eval_script(void* this_, void* Isolate, const char* data, int type, int length);
		static int64_t eval_caller(int64_t* a1);
	};

	struct minhook_keepalive
	{
		minhook_keepalive()
		{
			MH_Initialize();
		}
		~minhook_keepalive()
		{
			MH_Uninitialize();
		}
	};

	class hooking
	{
		friend hooks;
	public:
		explicit hooking();
		~hooking();

		void enable();
		void disable();
	private:
		bool m_enabled{};
		minhook_keepalive m_minhook_keepalive;

		WNDPROC m_og_wndproc;
#ifndef UNIVERSAL
		vmt_hook m_swapchain_hook;
#endif
	};

	inline hooking* g_hooking{};
}
