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
    class add_magazine : public command
    {
        using command::command;

        void on_call() override
        {
            auto c = unreal_engine::get_character();
            auto ct = unreal_engine::get_player_controller();
            auto wep = c->GetEquippedWeapon();

            if (!wep)
                return;

            if (!ct->HasAuthority()) return;

            SDK::FMagazine mag{};
            mag.Ammo = 30;
            mag.AmmoType = 1;
            wep->Server_AddMagazine(mag);
        }
    };

    static add_magazine _add_magazine("add_magazine", "Add Magazine", "");
}