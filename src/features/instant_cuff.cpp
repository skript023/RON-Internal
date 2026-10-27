#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	static void stop_montage(SDK::USkeletalMeshComponent* mesh, SDK::UAnimMontage* montage)
	{
		auto anim = mesh && montage ? mesh->GetAnimInstance() : nullptr;
		if (anim && anim->Montage_IsPlaying(montage))
			anim->Montage_Stop(0.1f, montage);
	}

	// Zipcuffing plays an animation together with the person being cuffed, and the arrest completes at a notify inside
	// it. So as soon as your zipcuffs start, the host completes the arrest itself, as Arrest All does. The animation
	// starts a moment later; it is known by the zipcuffs it plays with, stopped on both of you and ended, since stopping
	// it alone leaves the interaction waiting out its length with your hands empty.
	class instant_cuff : public looped_command
	{
		using looped_command::looped_command;

		weak_object<SDK::AZipcuffs> m_cuffs;
		// The interaction last ended, so each one is ended once.
		weak_object<SDK::APairedInteractionDriver> m_ended;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			if (!c || !ron::is_host())
				return;

			auto cuffs = m_cuffs.get();
			if (!cuffs)
			{
				auto item = c->InventoryComp ? c->InventoryComp->GetInventoryItemOfClass(SDK::AZipcuffs::StaticClass(), false) : nullptr;
				if (!item)
					return;
				cuffs = static_cast<SDK::AZipcuffs*>(item);
				m_cuffs = cuffs;
			}

			if (auto target = cuffs->TargetedCharacter; cuffs->bArresting && target && !target->IsArrested())
			{
				target->ArrestComplete(c, nullptr);
				cuffs->Server_ArrestCancelled();
			}

			auto driver = SDK::UInteractionsData::IsPairedInteractionPlayingOn(c);
			auto data = driver && driver->OptionalItem == cuffs && m_ended.get() != driver ? driver->InteractionData : nullptr;
			if (!data)
				return;

			stop_montage(c->Mesh, data->DriverMontage);
			stop_montage(c->Mesh1P, data->DriverMontage_FP);
			if (driver->Slave && driver->Slave->IsA(SDK::ACharacter::StaticClass()))
				stop_montage(static_cast<SDK::ACharacter*>(driver->Slave)->Mesh, data->SlaveMontage);

			m_ended = driver;
			driver->OnDriverInteractionFinished();
			driver->OnSlaveInteractionFinished();
		}
	};

	static instant_cuff _instant_cuff("instant_cuff", "Instant Cuff", "Zipcuffing someone finishes at once (host only)");
}
