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
#include "utility/weak_object.hpp"

namespace big
{
	class infinite_ammo : public looped_command
	{
		using looped_command::looped_command;

		// Every weapon this feature switched on, so turning it off clears them all, not only the one in hand.
		std::vector<weak_object<SDK::ABaseMagazineWeapon>> m_weapons;

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon(); wep && !wep->bInfiniteAmmo)
				{
					wep->bInfiniteAmmo = 1;
					m_weapons.emplace_back(wep);
				}
			}
		}

		void on_disable() override
		{
			for (auto& weapon : m_weapons)
			{
				if (auto wep = weapon.get())
				{
					if (wep->bInfiniteAmmo)
					{
						wep->bInfiniteAmmo = 0;
					}
				}
			}

			m_weapons.clear();
		}
	};

	static infinite_ammo _infinite_ammo("infinite_ammo", "Infinite Ammo", "Magazines never run dry");
}
