#include "../view.hpp"
#include "events/mission_event.hpp"

#include <astra/components.hpp>
#include <astra/host/canvas.hpp>

namespace big
{
	void view::mission_overlay()
	{
		if (!_mission_overlay.get_state())
			return;

		const auto rows = mission_event::overlay();
		if (!rows.empty())
			canvas::draw_overlay("Mission Tracker", rows);
	}

	void view::target_inspector()
	{
		if (!_target_inspector.get_state())
			return;

		if (const auto card = mission_event::inspector())
		{
			if (const int action = canvas::draw_inspector("Target Inspector", *card); action >= 0)
				mission_event::inspector_action(action);
		}
	}

	void view::collectibles_panel()
	{
		if (!_collectibles_window.get_state())
			return;

		bool open = true;
		if (canvas::begin_panel("Collectibles", &open, {.size = {460.f, 420.f}}))
		{
			static char search[64]{};
			astra::search_field("collectible_search", search, sizeof(search), "Search collectible name");

			const auto all = mission_event::collectibles();
			std::vector<const collectible_entry*> found;
			for (const auto& entry : all)
			{
				if (astra::matches_search(entry.name, search))
					found.push_back(&entry);
			}

			const char* actions[] = {"Teleport"};
			const auto clicked = astra::action_list("collectibles", static_cast<int>(found.size()), [&](int i) {
				return astra::action_row{found[i]->name, std::format("{:.0f} m away", found[i]->distance)};
			}, actions, {.count_label = "On this map", .empty_text = "No collectibles left on this map."});

			if (clicked.row >= 0 && clicked.action == 0)
				mission_event::teleport(found[clicked.row]->location);
		}
		canvas::end_panel();

		// The close button switches the feature off, the same as the menu row.
		if (!open)
			_collectibles_window.set_state(false);
	}
}
