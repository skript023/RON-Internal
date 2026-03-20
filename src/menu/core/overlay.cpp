#include "../view.hpp"
#include "utility/player.hpp"
#include <ui/canvas.hpp>

namespace big
{
	void view::draw_overlay()
	{
		if (!g_settings.window.overlay) return;

		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.5f);

		auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

		if (!g_settings.window.mouse_active) window_flags |= ImGuiWindowFlags_NoMouseInputs;

		/*if (ImGui::Begin("overlay", nullptr, window_flags))
		{
			ImGui::Text("Overlay Info");
			ImGui::Text("Resulution %dx%d", g_pointers->m_resolution->x, g_pointers->m_resolution->y);

			ImGui::Text("Energy: %.f/%.f", player::get_energy(), player::get_energy_max());
			ImGui::Text("Energy Efficiency: %.f", player::get_energy_efficiency());
		}
		ImGui::End();*/
	}
}