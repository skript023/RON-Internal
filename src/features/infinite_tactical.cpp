#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// Grenades, wedges and C2 charges are items of their own that leave the inventory once used. ReplenishAllGrenadeAmmo
	// would refill grenades but destroys the one in the air too, so a new item of the same kind is added for each one
	// thrown, set in a door or placed; one dropped or taken away otherwise is not replaced.
	static bool was_used(SDK::ABaseItem* item)
	{
		if (item->IsA(SDK::ABaseGrenade::StaticClass()))
			return static_cast<SDK::ABaseGrenade*>(item)->bUsed;
		if (item->IsA(SDK::ADoorJam::StaticClass()))
			return static_cast<SDK::ADoorJam*>(item)->bSet;
		if (item->IsA(SDK::AC2Explosive::StaticClass()))
			return static_cast<SDK::AC2Explosive*>(item)->LastPlacedC2Explosive != nullptr;
		return false;
	}

	// Chemlights and pepper spray count down an ammo resource instead, which the game itself can make unlimited.
	static SDK::UAmmoComponent* ammo_of(SDK::ABaseItem* item)
	{
		if (item->IsA(SDK::AChemlight::StaticClass()))
			return static_cast<SDK::AChemlight*>(item)->Ammo;
		if (item->IsA(SDK::APepperspray::StaticClass()))
			return static_cast<SDK::APepperspray*>(item)->AmmoComponent;
		return nullptr;
	}

	class infinite_tactical : public looped_command
	{
		using looped_command::looped_command;

		// The items held last tick, so a used one shows up as missing.
		std::vector<weak_object<SDK::ABaseItem>> m_held;
		// The resources made unlimited here, so turning off sets them back.
		std::vector<weak_object<SDK::UAmmoComponent>> m_unlimited;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (!c || !c->InventoryComp || !ron::is_host())
				return;

			std::vector<weak_object<SDK::ABaseItem>> held;
			for (auto item : c->InventoryComp->InventoryItems)
			{
				if (!item)
					continue;
				held.emplace_back(item);
				// Kept only once it took, so a resource that will not change is not added again every tick.
				if (auto ammo = ammo_of(item); ammo && !ammo->bUnlimited)
				{
					ammo->SetUnlimitedResource(true);
					if (ammo->bUnlimited)
						m_unlimited.emplace_back(ammo);
				}
			}

			for (const auto& entry : m_held)
			{
				auto item = entry.get();
				if (item && was_used(item) && std::find(held.begin(), held.end(), entry) == held.end())
					ron::give_item(c, item->Class);
			}
			m_held = std::move(held);
		}

		void on_disable() override
		{
			for (const auto& entry : m_unlimited)
			{
				if (auto ammo = entry.get())
					ammo->SetUnlimitedResource(false);
			}
			m_unlimited.clear();
			m_held.clear();
		}
	};

	static infinite_tactical _infinite_tactical("infinite_tactical", "Infinite Tactical Items",
	    "Grenades, wedges and C2 you use are replaced at once, and chemlights and pepper spray never run out (host only)");
}
