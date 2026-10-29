#include "commands/float_command.hpp"
#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weapon_backup.hpp"
#include "host_link.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	static float_command _fire_rate_mult("fire_rate_mult", "Fire Rate Multiplier", "", 1.f, 5.f, 2.f);

	// Each weapon's own time between shots, so the multiplier scales that and turning off puts it back.
	static weapon_backup<float> g_fire_rate_backup;

	static void speed_up(SDK::APlayerCharacter* c, float mult)
	{
		auto weapon = c->GetEquippedWeapon();
		if (!weapon)
			return;

		if (!g_fire_rate_backup.contains(weapon))
			g_fire_rate_backup.add(weapon, weapon->FireRate);

		mult = (std::max)(mult, 1.f);
		for (auto& [entry, fire_rate] : g_fire_rate_backup)
		{
			if (entry.get() == weapon)
				weapon->FireRate = fire_rate / mult;
		}
	}

	// The weapons of one pawn only, as the host may also speed up clients' weapons.
	static void slow_down(SDK::APlayerCharacter* c)
	{
		for (auto& [entry, fire_rate] : g_fire_rate_backup)
		{
			if (auto weapon = entry.get(); weapon && weapon->Owner == c)
				weapon->FireRate = fire_rate;
		}
	}

	// A weapon's FireRate is the time between its shots, such as 0.081 s for the F90, so it is divided; at most one shot
	// still goes per frame. The host checks every shot it is sent, so a client fires faster only when the host runs this
	// mod and speeds up its copy of the client's weapon as well.
	class rapid_fire : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (c && (ron::is_host() || host_link::ready()))
				speed_up(c, _fire_rate_mult.get_state());
		}

		void on_disable() override
		{
			for (auto& [entry, fire_rate] : g_fire_rate_backup)
			{
				if (auto weapon = entry.get())
					weapon->FireRate = fire_rate;
			}
			g_fire_rate_backup.clear();
		}
	};

	static rapid_fire _rapid_fire("rapid_fire", "Rapid Fire", "Automatic fire is faster; single shots still go as fast as you click (host only)");

	static host_link::client_toggle _rapid_fire_for_clients("rapid_fire"_hash, speed_up, slow_down, [] { return _fire_rate_mult.get_state(); });
}
