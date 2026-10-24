#pragma once
#include "commands/bool_command.hpp"
#include "task.hpp"
#include <astra/hud.hpp>

namespace big
{
	struct collectible_entry
	{
		std::string name;
		SDK::FVector location;
		float distance;
	};

	inline bool_command _mission_overlay("mission_overlay", "Mission Tracker", "Suspects, civilians, evidence, objectives, and score at a glance", false);
	inline bool_command _target_inspector("target_inspector", "Target Inspector", "A card for the person or door under your crosshair", false);
	inline bool_command _collectibles_window("collectibles_window", "Collectibles", "Every lore collectible on the map, with teleport", false);
	inline bool_command _objective_alerts("objective_alerts", "Objective Alerts", "A notification when an objective completes or fails", true);

	// Reads the mission on the game thread a few times a second; the render thread draws from copies of what it read.
	class mission_event
	{
	public:
		static task<void> run();

		static std::vector<astra::overlay_row> overlay();
		static std::optional<astra::inspector> inspector();
		static std::vector<collectible_entry> collectibles();

		// Called from the render thread; the action itself runs on the game thread.
		static void inspector_action(int index);
		static void teleport(const SDK::FVector& location);
	};
}
