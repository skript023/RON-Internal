#include "commands/float_command.hpp"
#include "commands/looped_command.hpp"
#include "utility/player.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	static float_command _hands_speed("hands_speed", "Hands Speed", "", 1.f, 5.f, 2.f);

	// Plays the montage the mesh is on now, such as a reload, at the given rate.
	static void speed_up(SDK::USkeletalMeshComponent* mesh, float rate)
	{
		auto anim = mesh ? mesh->GetAnimInstance() : nullptr;
		auto montage = anim ? anim->GetCurrentActiveMontage() : nullptr;
		if (montage && anim->Montage_GetPlayRate(montage) != rate)
			anim->Montage_SetPlayRate(montage, rate);
	}

	// A reload and a grenade throw are montages on the first person arms and the item, and the game moves on when their
	// notifies fire, so playing them faster finishes them sooner.
	class fast_hands : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());
			auto item = c ? c->GetEquippedItem() : nullptr;
			if (!item)
				return;

			const bool reloading = item->IsA(SDK::ABaseMagazineWeapon::StaticClass()) && static_cast<SDK::ABaseMagazineWeapon*>(item)->bReloading;
			const bool grenade = item->IsA(SDK::ABaseGrenade::StaticClass());
			if (!reloading && !grenade)
				return;

			const float rate = _hands_speed.get_state();
			speed_up(c->Mesh1P, rate);
			speed_up(item->ItemMesh, rate);
		}
	};

	static fast_hands _fast_hands("fast_hands", "Fast Reload & Throw", "Reloads and grenade throws play faster; the slider sets how much");
}
