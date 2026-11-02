#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "host_link.hpp"
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

	// The items each pawn held last tick, so a used one shows up as missing; the host may refill clients too.
	static std::vector<std::pair<weak_object<SDK::APlayerCharacter>, std::vector<weak_object<SDK::ABaseItem>>>> g_held;
	// The resources made unlimited here, so turning off sets them back.
	static std::vector<weak_object<SDK::UAmmoComponent>> g_unlimited;

	static void refill(SDK::APlayerCharacter* c, float)
	{
		if (!c->InventoryComp)
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
					g_unlimited.emplace_back(ammo);
			}
		}

		std::erase_if(g_held, [](const auto& entry) { return !entry.first.get(); });
		auto last = std::ranges::find_if(g_held, [c](const auto& entry) { return entry.first.get() == c; });
		if (last == g_held.end())
		{
			g_held.emplace_back(c, std::move(held));
			return;
		}

		for (const auto& entry : last->second)
		{
			auto item = entry.get();
			if (item && was_used(item) && std::find(held.begin(), held.end(), entry) == held.end())
				ron::give_item(c, item->Class);
		}
		last->second = std::move(held);
	}

	class infinite_tactical : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (c && ron::is_host())
				refill(c, 0.f);
		}

		void on_disable() override
		{
			for (const auto& entry : g_unlimited)
			{
				if (auto ammo = entry.get())
					ammo->SetUnlimitedResource(false);
			}
			g_unlimited.clear();
			g_held.clear();
		}
	};

	static infinite_tactical _infinite_tactical("infinite_tactical", "Infinite Tactical Items",
	    "Grenades, wedges and C2 you use are replaced at once, and chemlights and pepper spray never run out (host only)");

	static host_link::client_toggle _infinite_tactical_for_clients("infinite_tactical"_hash, refill, [](SDK::APlayerCharacter* c) {
		std::erase_if(g_held, [c](const auto& entry) { return entry.first.get() == c; });
	});
}
