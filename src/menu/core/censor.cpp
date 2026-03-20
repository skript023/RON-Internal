#include "view.hpp"
#include "pointers.hpp"

namespace big
{
	void view::censor()
	{
		if (g_settings.window.censor)
		{
            // Set the display size manually (1920x1080)
            ImVec2 display_size = ImVec2((float)g_pointers->m_resolution->x, (float)g_pointers->m_resolution->y);

            // Set window size (e.g., 200x100)
            ImVec2 window_size = ImVec2(200, 40);

            // Set window position to the bottom right (10px padding)
            ImGui::SetNextWindowPos(ImVec2(display_size.x - window_size.x - 20, display_size.y * 0.965f));
            ImGui::SetNextWindowSize(window_size);

            // Window background color to make sure it's fully opaque
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255)); // Opaque dark grey
            // Begin a new ImGui window
            if (ImGui::Begin("UID Censor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::Text("Neo Hacks UUID Censorship");
            }

            // End the window
            ImGui::End();
            ImGui::PopStyleColor();
		}
	}
}