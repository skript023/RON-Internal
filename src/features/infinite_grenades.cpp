#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// Each grenade is an item of its own that leaves the inventory once thrown. ReplenishAllGrenadeAmmo would refill
	// them but destroys the one in the air too, so a new grenade of the same kind is added for each one thrown.
	class infinite_grenades : public looped_command
	{
		using looped_command::looped_command;

		// The grenades held last tick, so a thrown one shows up as missing.
		std::vector<weak_object<SDK::ABaseGrenade>> m_held;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (!c || !c->InventoryComp || !ron::is_host())
				return;

			std::vector<weak_object<SDK::ABaseGrenade>> held;
			for (auto item : c->InventoryComp->InventoryItems)
			{
				if (item && item->IsA(SDK::ABaseGrenade::StaticClass()))
					held.emplace_back(static_cast<SDK::ABaseGrenade*>(item));
			}

			// Only a used one was thrown; grenades dropped or taken away otherwise are not replaced.
			for (const auto& entry : m_held)
			{
				auto grenade = entry.get();
				if (grenade && grenade->bUsed && std::find(held.begin(), held.end(), entry) == held.end())
					ron::give_item(c, grenade->Class);
			}
			m_held = std::move(held);
		}

		void on_disable() override
		{
			m_held.clear();
		}
	};

	static infinite_grenades _infinite_grenades("infinite_grenades", "Infinite Grenades", "Every grenade you throw is replaced at once (host only)");
}
