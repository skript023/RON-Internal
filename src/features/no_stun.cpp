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

	// Skips a call that starts a stun's effects on your own character. Which character a call is about depends on the
	// function, so each one says where to find it.
	class stun_effect : public process_event
	{
	public:
		using target_t = SDK::UObject* (*)(SDK::UObject* _this, void* parms);

		stun_effect(std::string name, target_t target) :
		    process_event(std::move(name)),
		    m_target(target)
		{
		}

	private:
		target_t m_target;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			auto c = unreal_engine::get_character();
			if (_no_stun.get_state() && c && m_target(_this, parms) == c)
				return eExecutionStatus::EXECUTION_INTERCEPTED;

			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	// What last started a stun, so the damage it deals in the same moment is known as the stun's own.
	static weak_object<SDK::AActor> g_stun_causer;

	// The RPC that starts a stun on the stunned player's own machine.
	static stun_effect _client_stun("Function ReadyOrNot.PlayerCharacter.Client_StartStun", [](SDK::UObject* _this, void* parms) -> SDK::UObject* {
		g_stun_causer = static_cast<SDK::Params::PlayerCharacter_Client_StartStun*>(parms)->StunCauser;
		return _this;
	});

	// The post-process component's handler for the character's stunned event, such as a flashbang's white screen.
	static stun_effect _stun_screen("Function ReadyOrNot.PlayerPostProcessing.OnPlayerStunned", [](SDK::UObject*, void* parms) -> SDK::UObject* {
		const auto p = static_cast<SDK::Params::PlayerPostProcessing_OnPlayerStunned*>(parms);
		g_stun_causer = p->DamageCauser;
		return p->Character;
	});

	// A flashbang also suppresses you, which blurs the screen. Only less-lethal suppression, so gunfire still does.
	static stun_effect _stun_suppression("Function ReadyOrNot.ReadyOrNotCharacter.Multicast_InflictSuppression", [](SDK::UObject* _this, void* parms) -> SDK::UObject* {
		return static_cast<SDK::Params::ReadyOrNotCharacter_Multicast_InflictSuppression*>(parms)->bLessLethal ? _this : nullptr;
	});

	// The stun's own damage, such as a flashbang's, starts the injury blur; damage from anything else still does.
	static stun_effect _stun_injury("Function ReadyOrNot.PlayerPostProcessing.OnDamageTaken", [](SDK::UObject*, void* parms) -> SDK::UObject* {
		const auto p = static_cast<SDK::Params::PlayerPostProcessing_OnDamageTaken*>(parms);
		return p->DamageCauser && p->DamageCauser == g_stun_causer.get() ? p->DamagedCharacter : nullptr;
	});

	// Pepper spray has its own handler, which only names the cause, so the character is the component's owner.
	static stun_effect _pepper_screen("Function ReadyOrNot.PlayerPostProcessing.OnPlayerPepperSprayed", [](SDK::UObject* _this, void*) -> SDK::UObject* {
		return static_cast<SDK::UPlayerPostProcessing*>(_this)->OwningCharacter;
	});
}
