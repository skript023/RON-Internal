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
    class slots : public command
    {
        using command::command;

        void on_call() override
        {
            /*auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character());
            auto ct = unreal_engine::get_player_controller();

            if (!ct || !c)
                return;

            auto wep = c->Slot;

            if (!wep)
                return;

            if (!ct->HasAuthority()) return;

            SDK::FMagazine mag{};
            mag.Ammo = 30;
            mag.AmmoType = 1;
            wep->Server_AddMagazine(mag);*/
        }
    };

    static slots _slots("slots", "Add Slots", "");
}