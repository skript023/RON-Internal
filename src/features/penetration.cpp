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
	// The weapon's own distance stays with the weapon; the loaded ammo's distance and level change with the ammo type.
	struct ammo_penetration
	{
		float PenetrationDistance;
		UC::uint8 PenetrationLevel;
	};

	static constexpr float ammo_distance = 100000;
	static constexpr UC::uint8 ammo_level = 10;
	static constexpr float weapon_distance = 10000;

	static weapon_backup<float> g_weapon_backup;
	static ammo_backup<ammo_penetration> g_ammo_backup;

	static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		if (!g_weapon_backup.contains(wep))
			g_weapon_backup.add(wep, wep->PenetrationDistance);

		// Ammo that already reads as this feature's own stats is never taken for the original.
		const auto& ammo = wep->CurrentAmmoType;
		if (ammo.PenetrationDistance == ammo_distance && ammo.PenetrationLevel == ammo_level)
			return;
		if (const int type = g_ammo_backup.loaded_type(wep); !g_ammo_backup.contains(wep, type))
			g_ammo_backup.add(wep, type, { ammo.PenetrationDistance, ammo.PenetrationLevel });
	}

	static void apply_peneration(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		wep->CurrentAmmoType.PenetrationDistance = ammo_distance;
		wep->CurrentAmmoType.PenetrationLevel = ammo_level;
		wep->PenetrationDistance = weapon_distance;
	}

	static void restore_all()
	{
		for (auto& [weapon, distance] : g_weapon_backup)
		{
			if (auto wep = weapon.get())
				wep->PenetrationDistance = distance;
		}
		g_weapon_backup.clear();

		g_ammo_backup.restore_loaded([](SDK::ABaseMagazineWeapon* wep, const ammo_penetration& data) {
			wep->CurrentAmmoType.PenetrationDistance = data.PenetrationDistance;
			wep->CurrentAmmoType.PenetrationLevel = data.PenetrationLevel;
		});
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
