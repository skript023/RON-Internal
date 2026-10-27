#include "commands/float_command.hpp"
#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weapon_backup.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	static float_command _fire_rate_mult("fire_rate_mult", "Fire Rate Multiplier", "", 1.f, 5.f, 2.f);

	// Each weapon's own time between shots, so the multiplier scales that and turning off puts it back.
	static weapon_backup<float> g_fire_rate_backup;

	// A weapon's FireRate is the time between its shots, such as 0.081 s for the F90, so it is divided; at most one shot
	// still goes per frame. The host checks every shot it is sent, so only the host fires faster.
	class rapid_fire : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			auto weapon = c && ron::is_host() ? c->GetEquippedWeapon() : nullptr;
			if (!weapon)
				return;

			if (!g_fire_rate_backup.contains(weapon))
				g_fire_rate_backup.add(weapon, weapon->FireRate);

			const float mult = (std::max)(_fire_rate_mult.get_state(), 1.f);
			for (auto& [entry, fire_rate] : g_fire_rate_backup)
			{
				if (entry.get() == weapon)
					weapon->FireRate = fire_rate / mult;
			}
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
}
