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


namespace big
{
	struct peneration
	{
		float CPenetrationDistance;
		UC::uint8 CPenetrationLevel;
		float PenetrationDistance;
	};

	static std::unordered_map<SDK::ABaseMagazineWeapon*, peneration> g_backup;

	static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		// kalau belum pernah dibackup
		if (g_backup.find(wep) == g_backup.end())
		{
			g_backup[wep] = {
				wep->CurrentAmmoType.PenetrationDistance,
				wep->CurrentAmmoType.PenetrationLevel,
				wep->PenetrationDistance
			};
		}
	}

	static void apply_peneration(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		wep->CurrentAmmoType.PenetrationDistance = 100000;
		wep->CurrentAmmoType.PenetrationLevel = 10;
		wep->PenetrationDistance = 10000; // Move this
	}

	static void restore_all()
	{
		for (auto& [wep, data] : g_backup)
		{
			if (!wep) continue;

			wep->CurrentAmmoType.PenetrationDistance = data.CPenetrationDistance;
			wep->CurrentAmmoType.PenetrationLevel = data.CPenetrationLevel;
			wep->PenetrationDistance = data.CPenetrationDistance; // Move this
		}

		g_backup.clear();
	}

	class no_recoil : public looped_command
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
					apply_peneration(wep);
				}
			}
		}

		void on_disable() override
		{
			restore_all();
		}
	};

	static no_recoil _no_recoil("no_recoil", "No Spread", "");
}