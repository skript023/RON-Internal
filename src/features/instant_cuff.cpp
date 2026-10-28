#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "host_link.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	static void stop_montage(SDK::USkeletalMeshComponent* mesh, SDK::UAnimMontage* montage)
	{
		auto anim = mesh && montage ? mesh->GetAnimInstance() : nullptr;
		if (anim && anim->Montage_IsPlaying(montage))
			anim->Montage_Stop(0.1f, montage);
	}

	// The interaction each pawn last ended, so each one is ended once; the host may cuff for clients too.
	static std::vector<std::pair<weak_object<SDK::APlayerCharacter>, weak_object<SDK::APairedInteractionDriver>>> g_ended;

	// Zipcuffing plays an animation together with the person being cuffed, and the arrest completes at a notify inside
	// it. So as soon as the zipcuffs start, the host completes the arrest itself, as Arrest All does. The animation
	// starts a moment later; it is known by the zipcuffs it plays with, stopped on both and ended, since stopping it
	// alone leaves the interaction waiting out its length with empty hands.
	static void cuff_at_once(SDK::APlayerCharacter* c, float)
	{
		auto item = c->InventoryComp ? c->InventoryComp->GetInventoryItemOfClass(SDK::AZipcuffs::StaticClass(), false) : nullptr;
		if (!item)
			return;
		auto cuffs = static_cast<SDK::AZipcuffs*>(item);

		if (auto target = cuffs->TargetedCharacter; cuffs->bArresting && target && !target->IsArrested())
		{
			target->ArrestComplete(c, nullptr);
			cuffs->Server_ArrestCancelled();
		}

		std::erase_if(g_ended, [](const auto& entry) { return !entry.first.get(); });
		auto ended = std::ranges::find_if(g_ended, [c](const auto& entry) { return entry.first.get() == c; });

		auto driver = SDK::UInteractionsData::IsPairedInteractionPlayingOn(c);
		const bool done = ended != g_ended.end() && ended->second.get() == driver;
		auto data = driver && driver->OptionalItem == cuffs && !done ? driver->InteractionData : nullptr;
		if (!data)
			return;

		stop_montage(c->Mesh, data->DriverMontage);
		stop_montage(c->Mesh1P, data->DriverMontage_FP);
		if (driver->Slave && driver->Slave->IsA(SDK::ACharacter::StaticClass()))
			stop_montage(static_cast<SDK::ACharacter*>(driver->Slave)->Mesh, data->SlaveMontage);

		if (ended != g_ended.end())
			ended->second = driver;
		else
			g_ended.emplace_back(c, driver);
		driver->OnDriverInteractionFinished();
		driver->OnSlaveInteractionFinished();
	}

	class instant_cuff : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (c && ron::is_host())
				cuff_at_once(c, 0.f);
		}
	};

	static instant_cuff _instant_cuff("instant_cuff", "Instant Cuff", "Zipcuffing someone finishes at once (host only)");

	static host_link::client_toggle _instant_cuff_for_clients("instant_cuff"_hash, cuff_at_once);
}
