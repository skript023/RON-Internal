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
	struct damage_backup
	{
		float DismembermentDamage;
		float Damage;
		float DurabilityDamage;
	};

	static weapon_backup<damage_backup> g_backup;

	class damage : public looped_command
	{
		using looped_command::looped_command;

		// Kept per weapon, so switching weapons while the feature is on still puts each one back afterwards.
		static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
		{
			if (wep && !g_backup.contains(wep))
				g_backup.add(wep, { wep->CurrentAmmoType.DismembermentDamage, wep->CurrentAmmoType.Damage, wep->CurrentAmmoType.DurabilityDamage });
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					backup_weapon(wep);

					wep->CurrentAmmoType.DismembermentDamage = 100000;
					wep->CurrentAmmoType.Damage = 10000;
					wep->CurrentAmmoType.DurabilityDamage = 10000;
				}
			}
		}

		void on_disable() override
		{
			for (auto& [weapon, data] : g_backup)
			{
				auto wep = weapon.get();
				if (!wep) continue;

				wep->CurrentAmmoType.DismembermentDamage = data.DismembermentDamage;
				wep->CurrentAmmoType.Damage = data.Damage;
				wep->CurrentAmmoType.DurabilityDamage = data.DurabilityDamage;
			}

			g_backup.clear();
		}
	};

	static damage _damage("damage", "Instant Kill", "Every hit from your weapon is lethal");
}
