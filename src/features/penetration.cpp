#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"
#include "utility/weapon_backup.hpp"

namespace big
{
	struct peneration
	{
		float CPenetrationDistance;
		UC::uint8 CPenetrationLevel;
		float RicochetChance;
	};

	static weapon_backup<peneration> g_backup;

	static float_command _penetration_mult("penetration_mult", "Penetration Multiplier",
	    "How many times further than normal your bullets go through walls and doors", 1.f, 100.f, 100.f);

	static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		// kalau belum pernah dibackup
		if (!g_backup.contains(wep))
		{
			g_backup.add(wep, {
				wep->CurrentAmmoType.PenetrationDistance,
				wep->CurrentAmmoType.PenetrationLevel,
				wep->CurrentAmmoType.RicochetChance
			});
		}
	}

	// The ammo's own penetration, from its backup, times the multiplier: a rifle's starts at 700, a pistol's at 100. The
	// highest level goes through any armour, and no bullet glances off. The gun's own PenetrationDistance, 30 for a rifle
	// and 20 for a pistol, is left alone: raised as well, bullets went up through floors but seldom hit anyone.
	static void apply_peneration(SDK::ABaseMagazineWeapon* wep)
	{
		for (auto& [weapon, original] : g_backup)
		{
			if (weapon.get() != wep)
				continue;

			const float mult = _penetration_mult.get_state();
			wep->CurrentAmmoType.PenetrationDistance = original.CPenetrationDistance * mult;
			wep->CurrentAmmoType.PenetrationLevel = 10;
			wep->CurrentAmmoType.RicochetChance = 0.f;
			return;
		}
	}

	static void restore_all()
	{
		for (auto& [weapon, data] : g_backup)
		{
			auto wep = weapon.get();
			if (!wep) continue;

			wep->CurrentAmmoType.PenetrationDistance = data.CPenetrationDistance;
			wep->CurrentAmmoType.PenetrationLevel = data.CPenetrationLevel;
			wep->CurrentAmmoType.RicochetChance = data.RicochetChance;
		}

		g_backup.clear();
	}

	class penetrate_wall : public looped_command
	{
		using looped_command::looped_command;

		void on_enable() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					backup_weapon(wep);
				}
			}
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					// A weapon switched to while the feature is on is backed up before it changes too.
					backup_weapon(wep);
					apply_peneration(wep);
				}
			}
		}

		void on_disable() override
		{
			restore_all();
		}
	};

	static penetrate_wall _penetrate_wall("penetrate_wall", "Wall Penetration", "Bullets go through walls and doors");
}