#include "../view.hpp"
#include "../core/submenu.hpp"

#include "fiber_pool.hpp"
#include "utility/player.hpp"
#include "input/input_service.hpp"
#include <custom_teleport/custom_teleport_service.hpp>

#include <astra/components.hpp>

namespace big
{
	static bool_command _bookmarks_window("bookmarks_window", "Saved Locations", "Your saved spots on this map, with teleport", false);

	// Locations are kept per map; one saved in another building means nothing here.
	static std::string current_map()
	{
		auto world = SDK::UWorld::GetWorld();
		return world ? world->GetName() : std::string("Default");
	}

	static void save_current_location(std::string const& name)
	{
		const auto coords = player::get_player_coords();
		const auto rotation = player::get_player_rotation();

		teleport_location location;
		location.name = name;
		location.x = coords.X;
		location.y = coords.Y;
		location.z = coords.Z;
		location.yaw = rotation.Yaw;
		location.pitch = rotation.Pitch;
		location.roll = rotation.Roll;

		g_custom_teleport_service.save_new_location(current_map(), location);
	}

	void view::teleport_submenu()
	{
		canvas::add_tab<regular_submenu>("Teleport", SubmenuTeleport, [](regular_submenu* sub)
		{
			sub->add_option<reguler_option>("Teleport Forward", "Moves you a few metres ahead", [] {
				g_fiber_pool->queue_job([] { player::teleport_forward(); });
			});
			sub->add_option<reguler_option>("Save Current Location", "Saves where you stand on this map under a name", [] {
				g_input_service.show("Location name", [](std::string const& input) {
					if (!input.empty())
						save_current_location(input);
				});
			});
			sub->add_option<bool_option<bool>>("bookmarks_window"_hash);
		});
	}

	void view::bookmarks_panel()
	{
		if (!_bookmarks_window.get_state())
			return;

		static bool loaded = false;
		if (!loaded)
		{
			g_custom_teleport_service.fetch_saved_locations();
			loaded = true;
		}

		const auto map = current_map();
		bool open = true;
		if (canvas::begin_panel("Saved Locations", &open, {.size = {520.f, 460.f}}))
		{
			ImGui::TextUnformatted(map.c_str());

			static const std::vector<teleport_location> none;
			const auto it = g_custom_teleport_service.all_saved_locations.find(map);
			const auto& saved = it != g_custom_teleport_service.all_saved_locations.end() ? it->second : none;

			const char* actions[] = {"Teleport", "Delete"};
			const auto clicked = astra::action_list("bookmarks", static_cast<int>(saved.size()), [&](int i) {
				return astra::action_row{saved[i].name, std::format("X {:.0f}  Y {:.0f}  Z {:.0f}", saved[i].x, saved[i].y, saved[i].z)};
			}, actions, {.count_label = "Saved on this map", .empty_text = "Nothing saved here yet. Use Save Current Location."});

			if (clicked.row >= 0 && clicked.action == 0)
			{
				const auto location = saved[clicked.row];
				g_fiber_pool->queue_job([location] {
					player::set_player_coords(SDK::FVector(location.x, location.y, location.z + 100.f), SDK::FRotator(location.pitch, location.yaw, location.roll));
				});
			}
			else if (clicked.row >= 0 && clicked.action == 1)
			{
				const auto name = saved[clicked.row].name;
				g_custom_teleport_service.delete_saved_location(map, name);
			}
		}
		canvas::end_panel();

		if (!open)
			_bookmarks_window.set_state(false);
	}
}
