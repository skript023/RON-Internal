#include "utility/player.hpp"
#include "utility/weak_object.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "host_link.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	static void end_stuns(SDK::APlayerCharacter* c, float)
	{
		if (!c->IsStunned())
			return;

		for (int i = 1; i < static_cast<int>(SDK::EStunType::ST_MAX); i++)
		{
			const auto type = static_cast<SDK::EStunType>(i);
			if (c->IsStunnedWith(type))
				c->EndStun(type);
		}
	}

	// A stun also shakes the camera with one of the game's Camera_Stun shakes, and each grenade's bang with C_Trap_Explode,
	// both started in native code, so they are stopped on the next tick.
	static void stop_stun_shakes(SDK::APlayerCameraManager* camera)
	{
		auto shakes = camera ? camera->CachedCameraShakeMod : nullptr;
		if (!shakes)
			return;

		static std::unordered_map<SDK::UClass*, bool> stun_shakes;
		std::vector<SDK::UClass*> playing;
		for (const auto& active : shakes->ActiveShakes)
		{
			auto shake = active.ShakeInstance ? active.ShakeInstance->Class : nullptr;
			if (!shake)
				continue;

			auto it = stun_shakes.find(shake);
			if (it == stun_shakes.end())
			{
				const auto name = shake->GetName();
				it = stun_shakes.emplace(shake, name.starts_with("Camera_Stun_") || name == "C_Trap_Explode_C").first;
			}
			if (it->second)
				playing.push_back(shake);
		}
		// Stopping changes the list, so the shakes are stopped once it has been read.
		for (auto shake : playing)
			camera->StopAllInstancesOfCameraShake(shake, true);
	}

	// The host starts every stun in native code, so it cannot be blocked there; the host ends it on the next tick
	// instead. A client only stops its own screen effects, below, while the host's stun still slows it down, unless the
	// host runs this mod and ends the client's stuns too.
	class no_stun : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			if (c && controller && controller->HasAuthority())
				end_stuns(c, 0.f);
			if (controller)
				stop_stun_shakes(controller->PlayerCameraManager);
		}
	};

	static host_link::client_toggle _no_stun_for_clients("no_stun"_hash, end_stuns);

	static no_stun _no_stun("no_stun", "No Stun", "Flashbangs, gas, tasers, pepper spray and less-lethal rounds do not stun you; as a client only their screen effects stop");
	static bool_command _no_suppression("no_suppression", "No Suppression", "Gunfire and explosions near you do not blur or shake your screen", false);

	// Bleeding and the injury blur are started from native code and last a while, so their effects are stopped again on
	// every tick while you bleed or are hurt; the bleeding and the damage themselves go on.
	class no_hit_effects : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			auto screen = c ? c->PlayerPostProcessingComp : nullptr;
			if (!screen)
				return;

			if (c->BleedComponent && c->BleedComponent->IsBleeding())
				screen->StopBleedingEffect();
			if (c->GetCurrentHealth() < c->GetMaxHealth())
				screen->StopInjuryEffects();
		}
	};

	static no_hit_effects _no_hit_effects("no_hit_effects", "No Hit Effects", "Being hit or bleeding does not blur or redden your screen; you still take the damage");

	// Skips a call about your own character. Each function says which character a call is about, or nothing while the
	// feature that skips it is off.
	class screen_effect : public process_event
	{
	public:
		using target_t = SDK::UObject* (*)(SDK::UObject* _this, void* parms);

		screen_effect(std::string name, target_t target) :
		    process_event(std::move(name)),
		    m_target(target)
		{
		}

	private:
		target_t m_target;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			auto c = unreal_engine::get_character();
			if (c && m_target(_this, parms) == c)
				return eExecutionStatus::EXECUTION_INTERCEPTED;

			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	// What last started a stun, so the damage it deals in the same moment is known as the stun's own.
	static weak_object<SDK::AActor> g_stun_causer;

	// The RPC that starts a stun on the stunned player's own machine.
	static screen_effect _client_stun("Function ReadyOrNot.PlayerCharacter.Client_StartStun", [](SDK::UObject* _this, void* parms) -> SDK::UObject* {
		g_stun_causer = static_cast<SDK::Params::PlayerCharacter_Client_StartStun*>(parms)->StunCauser;
		return _no_stun.get_state() ? _this : nullptr;
	});

	// The post-process component's handler for the character's stunned event, such as a flashbang's white screen.
	static screen_effect _stun_screen("Function ReadyOrNot.PlayerPostProcessing.OnPlayerStunned", [](SDK::UObject*, void* parms) -> SDK::UObject* {
		const auto p = static_cast<SDK::Params::PlayerPostProcessing_OnPlayerStunned*>(parms);
		g_stun_causer = p->DamageCauser;
		return _no_stun.get_state() ? p->Character : nullptr;
	});

	// Pepper spray has its own handler, which only names the cause, so the character is the component's owner.
	static screen_effect _pepper_screen("Function ReadyOrNot.PlayerPostProcessing.OnPlayerPepperSprayed", [](SDK::UObject* _this, void*) -> SDK::UObject* {
		return _no_stun.get_state() ? static_cast<SDK::UPlayerPostProcessing*>(_this)->OwningCharacter : nullptr;
	});

	// Damage starts the injury blur. No Stun skips it for the stun's own damage, such as a flashbang's, and No Hit Effects
	// for all of it.
	static screen_effect _injury_screen("Function ReadyOrNot.PlayerPostProcessing.OnDamageTaken", [](SDK::UObject*, void* parms) -> SDK::UObject* {
		const auto p = static_cast<SDK::Params::PlayerPostProcessing_OnDamageTaken*>(parms);
		const bool stun_damage = p->DamageCauser && p->DamageCauser == g_stun_causer.get();
		return _no_hit_effects.get_state() || (_no_stun.get_state() && stun_damage) ? p->DamagedCharacter : nullptr;
	});

	// A hit's other screen effects: the details of the damage, such as an armour or helmet hit, and the bullet's impact.
	// Neither names the character, so it is the component's owner.
	static SDK::UObject* hit_screen(SDK::UObject* _this, void*)
	{
		return _no_hit_effects.get_state() ? static_cast<SDK::UPlayerPostProcessing*>(_this)->OwningCharacter : nullptr;
	}

	static screen_effect _hit_details("Function ReadyOrNot.PlayerPostProcessing.OnDamageTakenDetails", hit_screen);
	static screen_effect _hit_impact("Function ReadyOrNot.PlayerPostProcessing.OnBulletImpact", hit_screen);

	// A hit also starts screen effects from native code, such as the red of being shot and the side it came from, but
	// each one first asks its requirements whether to start; No Hit Effects answers no for yours.
	static SDK::UObject* hit_requirement(SDK::UObject* _this, void* parms)
	{
		if (!_no_hit_effects.get_state())
			return nullptr;

		static_cast<SDK::Params::PostProcessRequirement_EnablePostProcessEffect*>(parms)->ReturnValue = false;
		return static_cast<SDK::UPostProcessRequirement*>(_this)->PlayerCharacter;
	}

	static screen_effect _hit_requirements[] = {
	    {"Function PPR_WasOwningPlayerShot.PPR_WasOwningPlayerShot_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WasShotInHead.PPR_WasShotInHead_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WereLimbsShot.PPR_WereLimbsShot_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_BulletBlockedBySAPIPlate.PPR_BulletBlockedBySAPIPlate_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_BulletNotBlockedBySAPIPlate.PPR_BulletNotBlockedBySAPIPlate_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WasPlayerHitFromFront.PPR_WasPlayerHitFromFront_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WasPlayerHitFromBack.PPR_WasPlayerHitFromBack_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WasPlayerHitFromLeft.PPR_WasPlayerHitFromLeft_C.EnablePostProcessEffect", hit_requirement},
	    {"Function PPR_WasPlayerHitFromRight.PPR_WasPlayerHitFromRight_C.EnablePostProcessEffect", hit_requirement},
	};

	// A hit also shakes the camera with one of the game's Camera_Hit shakes, sent to your controller; other shakes, such
	// as holstering a weapon, still play. The character is returned because it is what screen_effect compares with.
	static screen_effect _hit_shake("Function Engine.PlayerController.ClientStartCameraShake", [](SDK::UObject* _this, void* parms) -> SDK::UObject* {
		if (!_no_hit_effects.get_state() || _this != unreal_engine::get_player_controller())
			return nullptr;

		auto shake = static_cast<SDK::Params::PlayerController_ClientStartCameraShake*>(parms)->Shake.Get();
		if (!shake)
			return nullptr;

		static std::unordered_map<SDK::UClass*, bool> hit_shakes;
		auto it = hit_shakes.find(shake);
		if (it == hit_shakes.end())
			it = hit_shakes.emplace(shake, shake->GetName().starts_with("Camera_Hit_")).first;
		return it->second ? unreal_engine::get_character() : nullptr;
	});

	// Suppression blurs and shakes your screen: No Stun skips the less-lethal kind, such as a flashbang's, and No
	// Suppression all of it. Both RPCs, with and without line of sight, take the same parameters.
	template <typename Params>
	static SDK::UObject* suppression(SDK::UObject* _this, void* parms)
	{
		const bool less_lethal = static_cast<Params*>(parms)->bLessLethal;
		return _no_suppression.get_state() || (_no_stun.get_state() && less_lethal) ? _this : nullptr;
	}

	static screen_effect _suppression("Function ReadyOrNot.ReadyOrNotCharacter.Multicast_InflictSuppression",
	    suppression<SDK::Params::ReadyOrNotCharacter_Multicast_InflictSuppression>);
	static screen_effect _suppression_unseen("Function ReadyOrNot.ReadyOrNotCharacter.Multicast_InflictSuppression_NoLineOfSight",
	    suppression<SDK::Params::ReadyOrNotCharacter_Multicast_InflictSuppression_NoLineOfSight>);

	// Gunfire's suppression reaches the screen without one of the RPCs, so No Suppression also skips the handler that
	// draws it, and the screen shake that comes with it. That RPC plays whatever shake it is sent, so other shakes sent
	// the same way, such as an explosion's, are skipped too.
	static screen_effect _suppression_screen("Function ReadyOrNot.PlayerPostProcessing.OnSupression", [](SDK::UObject* _this, void*) -> SDK::UObject* {
		return _no_suppression.get_state() ? static_cast<SDK::UPlayerPostProcessing*>(_this)->OwningCharacter : nullptr;
	});
	static screen_effect _suppression_shake("Function ReadyOrNot.ReadyOrNotCharacter.Client_PlayScreenShake", [](SDK::UObject* _this, void*) -> SDK::UObject* {
		return _no_suppression.get_state() ? _this : nullptr;
	});
}
