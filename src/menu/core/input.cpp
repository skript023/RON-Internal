#include "../view.hpp"
#include "pointers.hpp"
#include "task_mgr.hpp"

#include "imgui.h"
#include "input/input_service.hpp"
#include "utility/unreal_engine_utility.hpp"

namespace big
{
	void view::draw_input()
	{
		// The player's input is switched on the game thread, once when the box opens and once when it closes.
		static bool was_open = false;
		const bool open = g_settings.window.input;
		if (open != was_open)
		{
			was_open = open;
			async_run([open] {
				if (open)
					unreal_engine::disable_input();
				else
					unreal_engine::enable_input();
			});
		}

		if (!open)
			return;

		float screen_x = (float)g_pointers->m_resolution->x;
		float screen_y = (float)g_pointers->m_resolution->y;

		ImGui::SetNextWindowPos(ImVec2(screen_x * 0.25f, screen_y * 0.2f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.65f);
		ImGui::SetNextWindowSize({ screen_x * 0.5f, -1 });

		if (ImGui::Begin("Input", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10.f, 15.f });
			ImGui::TextUnformatted(g_input_service.m_title);
			// set focus by default on input box
			ImGui::SetKeyboardFocusHere(0);
			ImGui::SetNextItemWidth((screen_x * 0.5f) - 30.f);
			if (ImGui::InputText("##Input Here", g_input_service.m_input, IM_ARRAYSIZE(g_input_service.m_input), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				g_input_service.execute();
			}

			ImGui::Text("You entered: %s", g_input_service.m_input);

			ImGui::PopStyleVar();
		}
		ImGui::End();
	}
}
