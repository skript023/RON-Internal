#pragma once
#include "task.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// One achievement as Steam itself lists it: the api name is what SetAchievement takes, and the display name is what
	// the player reads in the Steam client.
	struct steam_achievement
	{
		std::string api_name;
		std::string display;
		bool unlocked = false;
	};

	// Steam's own achievement list, read on the game thread while the menu shows it, with the unlocks going through
	// Steam as well: the game's own per-achievement calls answer as if they worked and change nothing.
	class achievement_event
	{
	public:
		static task<void> run();

		// Called from the render thread.
		static std::vector<steam_achievement> all();

		static void unlock_steam(std::string api_name);

		// The game's own calls, which take the whole list at once; these are what the game itself uses.
		static void unlock_all();
	};
}
