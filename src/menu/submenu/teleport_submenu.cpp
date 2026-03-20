#include "../view.hpp"
#include "../core/submenu.hpp"

#include "utility/player.hpp"
#include "input/input_service.hpp"
#include <custom_teleport/custom_teleport_service.hpp>

namespace big
{
	std::string category = "Default";

	void view::teleport_submenu()
	{
		canvas::add_tab<regular_submenu>("Teleport", SubmenuTeleport, [](regular_submenu* sub)
		{
			sub->add_option<reguler_option>("Teleport Forward", nullptr, [] {
				player::teleport_forward();
			});
			sub->add_option<sub_option>("Custom Teleport", nullptr, SubmenuCustomTeleport);
		});

		canvas::add_submenu<regular_submenu>("Custom Teleport", SubmenuCustomTeleport, [](regular_submenu* sub)
		{
			g_custom_teleport_service.fetch_saved_locations();
			sub->add_option<reguler_option>("Add Category", nullptr, [] {
				g_input_service.show("Input Category Name", [](std::string const& input) {
					teleport_location new_location;
					auto coords = player::get_player_coords();
					auto rotator = player::get_player_rotation();

					new_location.name = input;
					new_location.x = coords.X;
					new_location.y = coords.Y;
					new_location.z = coords.Z;
					new_location.yaw = rotator.Yaw;
					new_location.pitch = rotator.Pitch;
					new_location.roll = rotator.Roll;

					g_custom_teleport_service.save_new_location(input, new_location);
				});
			});

			for (auto& l : g_custom_teleport_service.all_saved_locations | std::ranges::views::keys)
			{
				sub->add_option<sub_option>(l.c_str(), nullptr, joaat(l), [=] {
					category = l;
				});

				canvas::add_submenu<regular_submenu>(l.c_str(), joaat(l), [l](regular_submenu* sub) {
					std::vector<teleport_location> current_list{};
					current_list = g_custom_teleport_service.all_saved_locations.at(l);

					sub->add_option<reguler_option>("Add Teleport", nullptr, [l] {
						g_input_service.show("Input Location Name", [](std::string const& input) {
							teleport_location new_location;
							auto coords = player::get_player_coords();
							auto rotator = player::get_player_rotation();

							new_location.name = input;
							new_location.x = coords.X;
							new_location.y = coords.Y;
							new_location.z = coords.Z;
							new_location.yaw = rotator.Yaw;
							new_location.pitch = rotator.Pitch;
							new_location.roll = rotator.Roll;

							g_custom_teleport_service.save_new_location(category, new_location);
						});
					});

					for (const auto& location : current_list)
					{
						sub->add_option<reguler_option>(location.name.c_str(), nullptr, [=] {
							player::set_player_coords(SDK::FVector(location.x, location.y, location.z + 100.f), SDK::FRotator(location.pitch, location.yaw, location.roll));
						});
					}
				});
			}
		});
	}
}