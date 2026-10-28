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
    // The host hands out ammo, so a client asks the host when it runs this mod.
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

            if (const int rounds = ron::give_magazine(unreal_engine::get_character()))
                notification::success(get_label(), std::format("Added a magazine of {} rounds.", rounds));
            else
                notification::warning(get_label(), "Hold a gun that takes magazines first.");
        }
    };

    static add_magazine _add_magazine("add_magazine", "Add Magazine", "Adds a full magazine of the loaded ammo to the gun in hand (host only)");

    static host_link::client_action _add_magazine_for_clients("add_magazine", "Add Magazine", [](SDK::APlayerCharacter* c, const std::string&) {
        return ron::give_magazine(c) > 0;
    });
}
