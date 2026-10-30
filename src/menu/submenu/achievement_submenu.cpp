#include "../view.hpp"
#include "../core/submenu.hpp"
#include "events/achievement_event.hpp"

namespace big
{
	void view::achievement_submenu()
	{
		canvas::add_tab<regular_submenu>("Achievements", SubmenuAchievements, [](regular_submenu* sub)
		{
			sub->add_option<reguler_option>("Unlock All", "Every achievement: the game's own call first, then whatever it left locked through Steam", [] {
				achievement_event::unlock_all();
			});

			// Steam's own list, one row per achievement: the name under it is the id Steam takes, and a click unlocks it
			// through Steam itself, which is what the game's per-achievement calls fail to do.
			const auto list = achievement_event::all();
			const auto unlocked = std::ranges::count_if(list, [](const steam_achievement& entry) { return entry.unlocked; });
			sub->add_option<reguler_option>(std::format("Steam: {} / {}", unlocked, list.size()).c_str(), "Read from Steam a moment ago; click a locked one to unlock it there");

			for (const auto& entry : list)
			{
				const auto label = std::format("{} {}", entry.unlocked ? "[x]" : "[ ]", entry.display);
				sub->add_option<reguler_option>(label.c_str(), entry.api_name.c_str(), [api = entry.api_name] {
					achievement_event::unlock_steam(api);
				});
			}

			if (list.empty())
				sub->add_option<reguler_option>("Nothing read yet", "Steam's list is read while this tab is open; open it again in a moment");
		});
	}
}
