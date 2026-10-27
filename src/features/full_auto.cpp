#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weapon_backup.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// Each gun's own fire mode, so turning off puts it back.
	static weapon_backup<SDK::EFireMode> g_fire_mode_backup;

	// A gun in single or burst is switched to automatic, which every gun fires at its own fire rate, pistols and pump
	// shotguns too; safe is left alone. The fire mode is the host's to set, so only the host fires this way.
	class full_auto : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			auto weapon = c && ron::is_host() ? c->GetEquippedWeapon() : nullptr;
			if (!weapon || (weapon->CurrentFireMode != SDK::EFireMode::FM_Single && weapon->CurrentFireMode != SDK::EFireMode::FM_Burst))
				return;

			if (!g_fire_mode_backup.contains(weapon))
				g_fire_mode_backup.add(weapon, weapon->CurrentFireMode);
			weapon->CurrentFireMode = SDK::EFireMode::FM_Auto;
		}

		void on_disable() override
		{
			for (auto& [entry, mode] : g_fire_mode_backup)
			{
				if (auto weapon = entry.get(); weapon && weapon->CurrentFireMode == SDK::EFireMode::FM_Auto)
					weapon->CurrentFireMode = mode;
			}
			g_fire_mode_backup.clear();
		}
	};

	static full_auto _full_auto("full_auto", "Full Auto", "Every gun fires as long as you hold the trigger, pistols and shotguns too (host only)");
}
