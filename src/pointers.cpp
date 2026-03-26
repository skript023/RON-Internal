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

		main_batch.add("GetViewPoint", "48 8B C4 48 89 58 10 48 89 68 18 56 57 41 57 48 81 EC ? ? ? ? 0F", [this](memory::handle ptr)
		{
			m_get_view_point = ptr.as<void*>();
		});

		main_batch.add("GetPlayerViewpoint", "48 89 6C 24 20 56 57 41 56 48 83 EC 40 48 8B 7A 10 4C 8B F1", [this](memory::handle ptr)
		{
			m_get_player_view_point = ptr.as<void*>();
		});

		main_batch.add("dwTlsIdx", "8B 0D ? ? ? ? FF 15 ? ? ? ? 41 8B 95 74 5B 00 00 48 8B D8 48 8B CB 48 8B 00 FF 10 48 8B CD E8", [this](memory::handle ptr)
		{
			m_tls_idx = ptr.add(2).rip().as<decltype(m_tls_idx)>();
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