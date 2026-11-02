#include <astra/host/canvas.hpp>
#include "pointers.hpp"
#include "utility/unreal_engine_utility.hpp"

namespace big
{
	bool canvas::platform_is_key_pressed(int key)
	{
		return unreal_engine::is_key_pressed(static_cast<std::uint16_t>(key));
	}
	bool canvas::platform_is_controller_pressed(int button)
	{
		return unreal_engine::is_controller_pressed(static_cast<std::uint16_t>(button));
	}
	Vector2 canvas::platform_resolution()
	{
		if (!g_pointers || !g_pointers->m_resolution)
			return {ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y};

		return {static_cast<float>(g_pointers->m_resolution->x), static_cast<float>(g_pointers->m_resolution->y)};
	}
	void canvas::platform_prepare_render()
	{
		// The game's DX11 back buffer is not sRGB.
		astra::set_srgb_output(false);
	}
}
