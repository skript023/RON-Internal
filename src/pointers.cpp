#include "common.hpp"
#include "pointers.hpp"

namespace big
{
	pointers::pointers(): main_batch("pointer_cache"), steam_batch("steam"), m_resolution(new iVector2(1920, 1080)), m_base_address(memory::module(nullptr).begin().as<uint64_t>())
	{
		main_batch.add("Return Address", "FF 23", [this](memory::handle ptr)
		{
			m_return_address = ptr.as<void*>();
		});

		main_batch.add("GetViewPoint", "48 8B C4 48 89 58 10 48 89 68 18 56 57 41 57 48 81 EC ? ? ? ? 0F 29 70 D8 48 8B FA 0F 29 78 C8 48 8B E9 44", [this](memory::handle ptr)
		{
			m_get_view_point = ptr.as<void*>();
		});

		main_batch.add("GetPlayerViewpoint", "48 89 5C 24 ? 55 56 41 56 48 8B EC 48 83 EC ? 48 8B F2", [this](memory::handle ptr)
		{
			m_get_player_view_point = ptr.as<void*>();
		});

		main_batch.add("Process Event", "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC", [this](memory::handle ptr)
		{
			m_process_event = ptr.as<decltype(m_process_event)>();
		});

		steam_batch.add("Swapchain", "48 8B ? ? ? ? ? 48 89 ? ? ? 48 8B ? ? ? ? ? 48 89 ? ? ? 89", [this](memory::handle ptr)
		{
			m_swapchain = ptr.add(3).rip().add(0x20).as<IDXGISwapChain**>();
		});

		main_batch.run(memory::module(nullptr));
		steam_batch.run(memory::module("gameoverlayrenderer64.dll"));

		this->m_hwnd = FindWindow(WINDOW_CLASS, WINDOW_NAME);
		if (!this->m_hwnd)
			throw std::runtime_error("Failed to find the game's window.");

		g_pointers = this;
	}

	pointers::~pointers()
	{
		g_pointers = nullptr;
	}

	void pointers::update()
	{
		main_batch.update();
		steam_batch.update();

		m_engine = SDK::UGameEngine::GetEngine();
	}
}