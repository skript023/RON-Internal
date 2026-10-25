#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
	static float_command _walk_mult("walk_mult", "Walk Multiplier", "", 1.f, 100.f, 1.f);

	// The speeds the game itself last asked for, walking, fast walking or moving slowly, so each is scaled rather than
	// replaced and put back when the feature turns off.
	static float g_walk = 0.f;
	static float g_crouch = 0.f;
	// Set while this feature sends speeds, so its own call is not taken for the game's.
	static bool g_sending = false;

	class walk_speed : public looped_command
	{
		using looped_command::looped_command;

		// The pawn whose speeds were read, so a respawned pawn is read again.
		weak_object<SDK::APlayerCharacter> m_character;

		static void set_speeds(SDK::APlayerCharacter* c, float walk, float crouch)
		{
			auto controller = unreal_engine::get_player_controller();
			g_sending = true;
			if (controller && controller->HasAuthority())
				c->Server_SetWalkSpeed(walk, crouch);
			else
				c->Client_SetWalkSpeed(walk, crouch);
			g_sending = false;
		}

		void restore()
		{
			if (auto c = m_character.get())
				set_speeds(c, g_walk, g_crouch);
			m_character = {};
		}

		void on_tick() override
		{
			auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());
			if (!c || !c->CharacterMovement)
				return;

			auto movement = c->CharacterMovement;
			if (m_character.get() != c)
			{
				restore();
				if (movement->MaxWalkSpeed <= 0.f)
					return;
				m_character = c;
				g_walk = movement->MaxWalkSpeed;
				g_crouch = movement->MaxWalkSpeedCrouched;
			}

			const float mult = _walk_mult.get_state();
			const float walk = g_walk * mult;
			const float crouch = g_crouch * mult;
			// Sent again only once the game has changed them, rather than as a reliable call every frame.
			if (movement->MaxWalkSpeed != walk || movement->MaxWalkSpeedCrouched != crouch)
				set_speeds(c, walk, crouch);
		}

		void on_disable() override
		{
			restore();
		}
	};

	static walk_speed _walk_speed("walk_speed", "Walk Speed", "Multiplies how fast you walk, fast walk and crouch");

	// Every speed change the game makes, such as starting to fast walk, goes through this RPC.
	class game_walk_speed : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			if (!g_sending && _this == unreal_engine::get_character())
			{
				const auto p = static_cast<SDK::Params::PlayerCharacter_Server_SetWalkSpeed*>(parms);
				g_walk = p->newWalkSpeed;
				g_crouch = p->newCrouchWalkSpeed;
			}
			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	static game_walk_speed _game_walk_speed("Function ReadyOrNot.PlayerCharacter.Server_SetWalkSpeed");

	static float_command _noclip_speed("noclip_speed", "Noclip Speed", "", 1.f, 30.f, 8.f);

	// Flies you where you look, through walls. The game's own movement is stopped, so neither gravity nor walls hold you,
	// and the walking input it still reads moves you instead, turned to the camera so looking up flies you up. The host
	// decides where everyone is, so only the host can fly.
	class noclip : public looped_command
	{
		using looped_command::looped_command;

		// The pawn that was stopped, so turning off lets only that one walk again.
		weak_object<SDK::APlayerCharacter> m_character;

		void on_tick() override
		{
			auto c = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			if (!c || !c->CharacterMovement || !controller || !controller->HasAuthority() || !ron::is_active(c))
				return;

			if (c->CharacterMovement->MovementMode != SDK::EMovementMode::MOVE_None)
				c->CharacterMovement->DisableMovement();
			m_character = c;

			// The input lies flat along where you face; its forward part is turned up or down to where you look.
			const auto input = c->GetLastMovementInputVector();
			if (input.Magnitude() <= 0.0)
				return;

			const double yaw = controller->ControlRotation.Yaw * (M_PI / 180.0);
			const double pitch = controller->ControlRotation.Pitch * (M_PI / 180.0);
			const SDK::FVector flat(std::cos(yaw), std::sin(yaw), 0.0);
			const SDK::FVector right(-std::sin(yaw), std::cos(yaw), 0.0);
			const SDK::FVector look(flat.X * std::cos(pitch), flat.Y * std::cos(pitch), std::sin(pitch));
			const auto move = look * input.Dot(flat) + right * input.Dot(right);
			const double step = _noclip_speed.get_state() * 100.0 * SDK::UGameplayStatics::GetWorldDeltaSeconds(c);
			c->K2_SetActorLocation(c->K2_GetActorLocation() + move * step, false, nullptr, true);
		}

		// A pawn that went down or was cuffed meanwhile is left to the game.
		void on_disable() override
		{
			auto c = m_character.get();
			if (c && ron::is_active(c) && c->CharacterMovement && c->CharacterMovement->MovementMode == SDK::EMovementMode::MOVE_None)
				c->CharacterMovement->SetMovementMode(SDK::EMovementMode::MOVE_Walking, 0);
			m_character = {};
		}
	};

	static noclip _noclip("noclip", "Noclip", "Fly where you look, through walls; turning it off inside a wall can leave you stuck (host only)");
}