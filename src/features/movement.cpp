#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
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

	class walk_speed : public looped_command
	{
		using looped_command::looped_command;

		// The pawn whose speeds were read, so a respawned pawn is read again.
		weak_object<SDK::APlayerCharacter> m_character;
		float m_walk = 0.f;
		float m_crouch = 0.f;

		static void set_speeds(SDK::APlayerCharacter* c, float walk, float crouch)
		{
			auto controller = unreal_engine::get_player_controller();
			if (controller && controller->HasAuthority())
				c->Server_SetWalkSpeed(walk, crouch);
			else
				c->Client_SetWalkSpeed(walk, crouch);
		}

		void restore()
		{
			if (auto c = m_character.get())
				set_speeds(c, m_walk, m_crouch);
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
				m_walk = movement->MaxWalkSpeed;
				m_crouch = movement->MaxWalkSpeedCrouched;
			}

			const float mult = _walk_mult.get_state();
			const float walk = m_walk * mult;
			const float crouch = m_crouch * mult;
			// Sent again only once the game has changed them, rather than as a reliable call every frame.
			if (movement->MaxWalkSpeed != walk || movement->MaxWalkSpeedCrouched != crouch)
				set_speeds(c, walk, crouch);
		}

		void on_disable() override
		{
			restore();
		}
	};

	static walk_speed _walk_speed("walk_speed", "Walk Speed", "Multiplies how fast you walk and crouch");
}