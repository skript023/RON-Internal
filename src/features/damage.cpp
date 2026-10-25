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
#include "utility/ron.hpp"
#include "task_mgr.hpp"

namespace big
{
	struct damage_backup
	{
		float DismembermentDamage;
		float Damage;
		float DurabilityDamage;
	};

	static ammo_backup<damage_backup> g_backup;

	class damage : public looped_command
	{
		using looped_command::looped_command;

		static constexpr float lethal_dismemberment = 100000;
		static constexpr float lethal_damage = 10000;

		// Kept per weapon and loaded ammo type, so switching either while the feature is on still puts each one back.
		// Stats that already read as lethal are this feature's own, so they are never taken for the originals.
		static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
		{
			const auto& ammo = wep->CurrentAmmoType;
			if (ammo.Damage == lethal_damage && ammo.DismembermentDamage == lethal_dismemberment)
				return;

			if (const int type = g_backup.loaded_type(wep); !g_backup.contains(wep, type))
				g_backup.add(wep, type, { ammo.DismembermentDamage, ammo.Damage, ammo.DurabilityDamage });
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					backup_weapon(wep);

					wep->CurrentAmmoType.DismembermentDamage = lethal_dismemberment;
					wep->CurrentAmmoType.Damage = lethal_damage;
					wep->CurrentAmmoType.DurabilityDamage = lethal_damage;
				}
			}
		}

		void on_disable() override
		{
			g_backup.restore_loaded([](SDK::ABaseMagazineWeapon* wep, const damage_backup& data) {
				wep->CurrentAmmoType.DismembermentDamage = data.DismembermentDamage;
				wep->CurrentAmmoType.Damage = data.Damage;
				wep->CurrentAmmoType.DurabilityDamage = data.DurabilityDamage;
			});
		}
	};

	static damage _damage("damage", "Instant Kill", "Every hit from your weapon is lethal");

	class lethal_hits : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			if (!_damage.get_state() || !ron::is_host())
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto params = static_cast<SDK::Params::ReadyOrNotCharacter_Multicast_TakeDamage*>(parms);
			auto player = unreal_engine::get_character();
			auto victim = static_cast<SDK::AReadyOrNotCharacter*>(_this);
			if (!player || params->InstigatorCharacter != player || victim == player)
				return eExecutionStatus::EXECUTION_CONTINUE;
			if (!victim->IsA(SDK::ACyberneticCharacter::StaticClass()) || victim->IsA(SDK::ASWATCharacter::StaticClass()))
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto causer = params->DamageCauser;
			if (!causer || !causer->IsA(SDK::ABaseMagazineWeapon::StaticClass()))
			{
				// Logged once, in case the game blames bullets on something other than the gun.
				static bool logged = false;
				if (causer && !logged)
				{
					LOG(INFO) << "Instant Kill left a hit by " << causer->Class->GetName() << " as it was.";
					logged = true;
				}
				return eExecutionStatus::EXECUTION_CONTINUE;
			}

			async_run([target = weak_object<SDK::AReadyOrNotCharacter>(victim)] {
				if (auto character = target.get(); character && !character->IsDeadNotUnconscious())
					character->Kill();
			});
			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	static lethal_hits _lethal_hits("Function ReadyOrNot.ReadyOrNotCharacter.Multicast_TakeDamage");
}
