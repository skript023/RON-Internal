#include "utility/player.hpp"
#include "utility/weak_object.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	// The host starts every stun in native code, so it cannot be blocked there; the host ends it on the next tick
	// instead. A client only stops its own screen effects, below, while the host's stun still slows it down.
	class no_stun : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			if (!c || !controller || !controller->HasAuthority() || !c->IsStunned())
				return;

			for (int i = 1; i < static_cast<int>(SDK::EStunType::ST_MAX); i++)
			{
				const auto type = static_cast<SDK::EStunType>(i);
				if (c->IsStunnedWith(type))
					c->EndStun(type);
			}
		}
	};

	static no_stun _no_stun("no_stun", "No Stun", "Flashbangs, gas, tasers, pepper spray and less-lethal rounds do not stun you; as a client only their screen effects stop");
	static bool_command _no_suppression("no_suppression", "No Suppression", "Gunfire and explosions near you do not blur or shake your screen", false);

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

	// The stun's own damage, such as a flashbang's, starts the injury blur; damage from anything else still does.
	static screen_effect _stun_injury("Function ReadyOrNot.PlayerPostProcessing.OnDamageTaken", [](SDK::UObject*, void* parms) -> SDK::UObject* {
		const auto p = static_cast<SDK::Params::PlayerPostProcessing_OnDamageTaken*>(parms);
		return _no_stun.get_state() && p->DamageCauser && p->DamageCauser == g_stun_causer.get() ? p->DamagedCharacter : nullptr;
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
