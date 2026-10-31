#pragma once
#include "commands/bool_command.hpp"
#include "task.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	inline bool_command _player_alerts("player_alerts", "Player Alerts", "A notification when someone joins or leaves, or a teammate goes down", true);

	// The career stats in a panel over the game, the way the mission tracker is drawn, so a stat can be set without
	// walking the menu's lists.
	inline bool_command _career_stats_overlay("career_stats_overlay", "Career Stats", "Every career stat in a panel over the game, with a box to set it", false);

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
		add_magazine,
		godmode,
		mute,
		kick
	};

	// One career stat as the last read on the game thread saw it.
	struct stat_info
	{
		std::string id;
		std::string label; // What the game calls it, such as Suspects Killed
		int type = 0;
		bool is_time = false;
		bool numeric = true;
		std::int64_t value = 0;
		std::string text;
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

		// A player's name as this client shows it, in the lobby's player list and in game. Your own name goes to both
		// and an empty one puts back the name you had; another player's name is the host's to set, for every client.
		// Called from the render thread; the rename itself runs on the game thread.
		static void rename(weak_object<SDK::APlayerState> state, const std::string& name);

		// The career stats as last read, and setting one of them. Reading asks the game thread for a fresh copy; the
		// write runs there too. An average is refused, since its value is the write divided by the samples behind it.
		static std::vector<stat_info> stats();
		static void set_stat(const stat_info& stat, const std::string& value);
		// The names a text stat can hold, read from the same loadout lists the give item panel shows; empty for stats
		// that are free text, such as an armour name.
		static std::vector<std::string> stat_choices(const std::string& id);
	};
}
