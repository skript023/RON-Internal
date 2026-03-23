#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
	static number_command<float> _walk_mult("walk_mult", "Walk Multiplier", "", 240.f, 1000.f, 240.f);

	class walk_speed : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();

			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (controller->HasAuthority())
					c->Server_SetWalkSpeed(240.0f * _walk_mult.get_state(), 240.0f * _walk_mult.get_state()); // 240 is default walk speed
				else
					c->Client_SetWalkSpeed(240.0f * _walk_mult.get_state(), 240.0f * _walk_mult.get_state()); // 240 is default walk speed
			}
		}

		void on_disable() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());

			if (c && controller)
			{
				if (controller->HasAuthority())
					c->Server_SetWalkSpeed(240.0f * _walk_mult.get_state(), 240.0f * _walk_mult.get_state()); // 240 is default walk speed
				else
					c->Client_SetWalkSpeed(240.0f * _walk_mult.get_state(), 240.0f * _walk_mult.get_state()); // 240 is default walk speed
			}
		}
	};

	static walk_speed _walk_speed("walk_speed", "Walk Speed", "");
}