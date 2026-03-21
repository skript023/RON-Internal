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

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				misc::set_bit(c->bGodMode, 1);
				//c->Server_ToggleGodMode();
			}
		}

		void on_disable() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (misc::has_bit_set(c->bGodMode, 1))
				{
					misc::clear_bit(c->bGodMode, 1);
					//c->Server_ToggleGodMode();
				}
			}
		}
	};

	static godmode _godmode("godmode", "Godmode", "");
}