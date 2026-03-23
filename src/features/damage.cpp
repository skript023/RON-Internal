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
	class damage : public looped_command
	{
		using looped_command::looped_command;

		float DismembermentDamage;
		float Damage;
		float DurabilityDamage;

		void on_enable() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					this->DismembermentDamage = wep->CurrentAmmoType.DismembermentDamage;
					this->Damage = wep->CurrentAmmoType.Damage;
					this->DurabilityDamage = wep->CurrentAmmoType.DurabilityDamage;
				}
			}
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					wep->CurrentAmmoType.DismembermentDamage = 100000;
					wep->CurrentAmmoType.Damage = 10000;
					wep->CurrentAmmoType.DurabilityDamage = 10000;
				}
			}
		}

		void on_disable() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					wep->CurrentAmmoType.DismembermentDamage = this->DismembermentDamage;
					wep->CurrentAmmoType.Damage = this->Damage;
					wep->CurrentAmmoType.DurabilityDamage = this->DurabilityDamage;
				}
			}
		}
	};

	static damage _damage("damage", "Instant Kill", "");
}