#pragma once
#include "task.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// One player in the lobby as the last read on the game thread saw them.
	struct player_info
	{
		weak_object<SDK::APlayerState> state;
		std::string name;
		bool local = false;
		bool host = false;
		bool spectated = false;
		int ping = 0;
		std::string platform;

		// From the game's own player list, so empty until that list has been opened once.
		std::string id;
		bool muted = false;

		// Left as they are while the player has no character, such as between missions.
		bool has_character = false;
		SDK::EPlayerHealthStatus status = SDK::EPlayerHealthStatus::HS_NotAvailable;
		float health = 0.f;
		float max_health = 0.f;
		float head = 0.f;
		float left_arm = 0.f;
		float right_arm = 0.f;
		float left_leg = 0.f;
		float right_leg = 0.f;
		std::string item;
		int ammo = -1;
		int capacity = 0;
		int magazines = 0;
		float distance = -1.f;
		bool godmode = false;
	};

	enum class player_action
	{
		teleport,
		spectate,
		heal,
		refill_ammo,
		godmode,
		mute,
		kick
	};

	// Reads the lobby on the game thread a few times a second; the menu draws from copies of what it read.
	class player_event
	{
	public:
		static task<void> run();

		static std::vector<player_info> players();
		static bool is_spectating();

		// The player the detail page shows. Called from the render thread.
		static void select(weak_object<SDK::APlayerState> state);
		static std::optional<player_info> selected();

		// Called from the render thread; the action itself runs on the game thread.
		static void act(player_action action);
		static void stop_spectating();
	};
}
