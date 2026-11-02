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
	// VelocityRecoilMultiplier belongs to No Recoil; changing it here too would let one feature restore the other's zero.
	struct spread_backup
	{
		SDK::FRotator SpreadPattern;
		SDK::FRotator PendingSpread;
		float SpreadReturnRate;
		float FirstShotSpread;
		float VelocitySpreadMultiplier;
		float ADSSpreadMultiplier;
		bool bIgnoreAmmoTypeSpread;
	};

	static weapon_backup<spread_backup> g_backup;

	static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		// kalau belum pernah dibackup
		if (!g_backup.contains(wep))
		{
			g_backup.add(wep, {
				wep->SpreadPattern,
				wep->PendingSpread,
				wep->SpreadReturnRate,
				wep->FirstShotSpread,
				wep->VelocitySpreadMultiplier,
				wep->ADSSpreadMultiplier,
				wep->bIgnoreAmmoTypeSpread
			});
		}
	}

	static void apply_no_spread(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		wep->SpreadPattern = SDK::FRotator();
		wep->PendingSpread = SDK::FRotator();
		wep->SpreadReturnRate = 0.0f;
		wep->FirstShotSpread = 0.0f;
		wep->VelocitySpreadMultiplier = 0.0f;
		wep->ADSSpreadMultiplier = 0.0f;
		wep->bIgnoreAmmoTypeSpread = true;
	}

	static void restore_all()
	{
		for (auto& [weapon, data] : g_backup)
		{
			auto wep = weapon.get();
			if (!wep) continue;

			wep->SpreadPattern = data.SpreadPattern;
			wep->PendingSpread = data.PendingSpread;
			wep->SpreadReturnRate = data.SpreadReturnRate;
			wep->FirstShotSpread = data.FirstShotSpread;
			wep->VelocitySpreadMultiplier = data.VelocitySpreadMultiplier;
			wep->ADSSpreadMultiplier = data.ADSSpreadMultiplier;
			wep->bIgnoreAmmoTypeSpread = data.bIgnoreAmmoTypeSpread;
		}

		g_backup.clear();
	}

	class no_spread : public looped_command
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
					apply_no_spread(wep);
				}
			}
		}

		void on_disable() override
		{
			restore_all();
		}
	};

	static no_spread _no_spread("no_spread", "No Spread", "Every shot goes where the sights point");
}
