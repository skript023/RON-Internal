#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "utility/ron.hpp"
#include "host_link.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
    // One more full magazine of the ammo the gun has loaded, sized to the gun's own capacity; the rounds, or 0 without a
    // gun that takes magazines. The host hands out ammo, so a client asks the host when it runs this mod.
    static int give_magazine(SDK::APlayerCharacter* c)
    {
        auto wep = c ? c->GetEquippedWeapon() : nullptr;
        // Launchers such as the M320 leave Capacity at 0 and hold one round per magazine in AmmoMax.
        const int capacity = !wep ? 0 : wep->Capacity > 0 ? wep->Capacity : static_cast<int>(wep->AmmoMax);
        if (capacity <= 0)
            return 0;

        SDK::FMagazine mag{};
        mag.Ammo = static_cast<UC::uint16>((std::min)(capacity, 0xFFFF));
        mag.AmmoType = wep->GetCurrentMagazine().AmmoType;
        wep->Server_AddMagazine(mag);
        return mag.Ammo;
    }

    class add_magazine : public command
    {
        using command::command;

        void on_call() override
        {
            if (!ron::is_host())
            {
                host_link::run(get_name(), get_label());
                return;
            }

            if (const int rounds = give_magazine(unreal_engine::get_character()))
                notification::success(get_label(), std::format("Added a magazine of {} rounds.", rounds));
            else
                notification::warning(get_label(), "Hold a gun that takes magazines first.");
        }
    };

    static add_magazine _add_magazine("add_magazine", "Add Magazine", "Adds a full magazine of the loaded ammo to the gun in hand (host only)");

    static host_link::client_action _add_magazine_for_clients("add_magazine", "Add Magazine", [](SDK::APlayerCharacter* c, const std::string&) {
        return give_magazine(c) > 0;
    });
}
