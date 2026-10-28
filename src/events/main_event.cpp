#include "main_event.hpp"
#include "script.hpp"
#include "pointers.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"
#include "settings/hotkeys.hpp"
#include "settings/settings.hpp"
#include "utility/ron.hpp"

namespace big
{
	// Toggles that only work as the host. One left on when you join someone else's lobby is turned off before any feature
	// ticks there, so it neither shows as on while doing nothing nor acts during the join.
	static void turn_off_host_only()
	{
		static constexpr joaat_t host_only[] = {"damage"_hash, "damage_multiplier"_hash, "rapid_fire"_hash, "full_auto"_hash, "noclip"_hash,
		    "instant_cuff"_hash, "infinite_tactical"_hash, "freeze_ai"_hash, "slow_motion"_hash, "no_penalty"_hash};

		// No controller, such as while a map loads, says nothing about who hosts.
		auto controller = unreal_engine::get_player_controller();
		if (!controller || controller->HasAuthority())
			return;

		std::string turned_off;
		for (const auto hash : host_only)
		{
			auto toggle = commands::get_bool_command(hash);
			if (!toggle || !toggle->get_state())
				continue;
			toggle->set_state(false);
			turned_off += (turned_off.empty() ? "" : ", ") + toggle->get_label();
		}
		if (!turned_off.empty())
			notification::info("Host Only", std::format("Turned off, as you joined as a client: {}.", turned_off));
	}

	static void update()
	{
		// Feature switches the Astra loader wrote to settings.json, then the per-feature hotkeys.
		settings::apply_external();
		hotkeys::tick(g_pointers->m_hwnd);

		turn_off_host_only();
		commands::run_looped_command();
	}

	// A fault in one feature skips this tick instead of ending every looped feature for the session.
	static void update_guarded()
	{
		TRY_CLAUSE
		{
			update();
		}
		EXCEPT_CLAUSE
	}

	void main_event::registration()
	{

	}
	task<void> main_event::run()
	{
		LOG(INFO) << "Main Event Registered";
		commands::enable_bool_commands();

		while (g_running)
		{
			update_guarded();
			co_await this_task::wait();
		}

		co_return;
	}
}
