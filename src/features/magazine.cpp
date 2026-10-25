#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "utility/ron.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
    // One more full magazine of the ammo the gun has loaded, sized to the gun's own capacity. The host hands out ammo,
    // so a client is told it only works there.
    class add_magazine : public command
    {
        using command::command;

        void on_call() override
        {
            if (!ron::require_host(get_label()))
                return;

            auto c = unreal_engine::get_character();
            auto wep = c ? c->GetEquippedWeapon() : nullptr;
            if (!wep || wep->Capacity <= 0)
            {
                notification::warning(get_label(), "Hold a gun that takes magazines first.");
                return;
            }

            SDK::FMagazine mag{};
            mag.Ammo = static_cast<UC::uint16>((std::min)(wep->Capacity, 0xFFFF));
            mag.AmmoType = wep->GetCurrentMagazine().AmmoType;
            wep->Server_AddMagazine(mag);

            notification::success(get_label(), std::format("Added a magazine of {} rounds.", mag.Ammo));
        }
    };

    static add_magazine _add_magazine("add_magazine", "Add Magazine", "Adds a full magazine of the loaded ammo to the gun in hand");
}