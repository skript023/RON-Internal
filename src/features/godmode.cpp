#include "script.hpp"
#include "utility/player.hpp"
#include "utility/weak_object.hpp"
#include "commands/looped_command.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
	class godmode : public looped_command
	{
		using looped_command::looped_command;

		// The pawn this feature last toggled, so a respawned pawn gets god mode again but the same one is not toggled twice.
		weak_object<SDK::APlayerCharacter> m_toggled;

		static void toggle(SDK::APlayerCharacter* c, SDK::APlayerController* controller)
		{
			if (controller->HasAuthority())
				c->Server_ToggleGodMode();
			else
				c->ToggleGodMode();
		}

		void on_enable() override
		{
			m_toggled = {};
			on_tick();
		}

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();

			if (!c || !controller || c->bGodMode)
				return;

			if (m_toggled.get() != c)
			{
				toggle(c, controller);
				m_toggled = c;
			}

			// The host decides damage, so its flag alone is enough even if the toggle did nothing.
			if (!c->bGodMode && controller->HasAuthority())
				c->bGodMode = 1;
		}

		void on_disable() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			m_toggled = {};

			if (!c || !controller || !c->bGodMode)
				return;

			toggle(c, controller);

			if (c->bGodMode && controller->HasAuthority())
				c->bGodMode = 0;
		}
	};

	static godmode _godmode("godmode", "Godmode", "You take no damage");
}
