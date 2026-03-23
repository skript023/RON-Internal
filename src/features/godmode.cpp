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
	class godmode : public looped_command
	{
		using looped_command::looped_command;

		void on_enable() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());

			if (c && controller)
			{
				misc::set_bit(c->bGodMode, 1);

				if (controller->HasAuthority())
					c->Server_ToggleGodMode();
				else
					c->ToggleGodMode();
			}
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				misc::set_bit(c->bGodMode, 1);
			}
		}

		void on_disable() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());

			if (c && controller)
			{
				if (misc::has_bit_set(c->bGodMode, 1))
				{
					misc::clear_bit(c->bGodMode, 1);
				}
				if (controller->HasAuthority())
					c->Server_ToggleGodMode();
				else
					c->ToggleGodMode();
			}
		}
	};

	static godmode _godmode("godmode", "Godmode", "");
}