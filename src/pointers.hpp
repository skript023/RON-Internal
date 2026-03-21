#pragma once
#include "common.hpp"
#include "function_types.hpp"
#include "memory/all.hpp"

#include "class/vector.hpp"
#include "class/render_base.hpp"
#include "class/command_base.hpp"

#include <SDK/Basic.hpp>
#include <SDK/Engine_classes.hpp>

namespace big
{
	class pointers
	{
		memory::pattern_batch main_batch;
		memory::pattern_batch steam_batch;
	public:
		explicit pointers();
		~pointers();

		void update();
		uint64_t m_base_address;
		IDXGISwapChain** m_swapchain;
	public:
		HWND m_hwnd{};
		GWorld** m_renderer{};
		SDK::UGameEngine* m_engine{};
		SDK::UWorld** m_world{};
		SDK::TUObjectArray** m_object_array{};
		void* m_name{};
		iVector2* m_resolution{};
		functions::process_event_t m_process_event{};
	public:
		void* m_eval_script;
		functions::eval_caller_t m_eval_caller;
		void* m_return_address;
		void* m_get_view_point;
		void* m_get_player_view_point;
	};

	inline pointers *g_pointers{};
}
