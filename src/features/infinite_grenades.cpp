#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// A new grenade of the given kind added to your inventory, which keeps it out of sight until it is drawn; one the
	// inventory turns down is destroyed rather than left lying where it was spawned.
	static void give_grenade(SDK::APlayerCharacter* c, SDK::UClass* type)
	{
		SDK::FTransform at{};
		at.Rotation.W = 1.0;
		at.Translation = c->K2_GetActorLocation();
		at.Scale3D = {1.0, 1.0, 1.0};
		constexpr auto scale = SDK::ESpawnActorScaleMethod::MultiplyWithRoot;
		auto actor = SDK::UGameplayStatics::BeginDeferredActorSpawnFromClass(c, type, at, SDK::ESpawnActorCollisionHandlingMethod::AlwaysSpawn, c, scale);
		auto item = actor ? static_cast<SDK::ABaseItem*>(SDK::UGameplayStatics::FinishSpawningActor(actor, at, scale)) : nullptr;
		if (item && !c->InventoryComp->AddInventoryItem(item))
			item->K2_DestroyActor();
	}

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
					give_grenade(c, grenade->Class);
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
