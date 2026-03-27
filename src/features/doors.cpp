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
    class unlock_doors : public command
    {
        using command::command;

        void on_call() override
        {
            auto world = SDK::UWorld::GetWorld(); if (!world) return;

            if (!world->GameState) return;

            auto gs = static_cast<SDK::AReadyOrNotGameState*>(world->GameState);

            if (!gs) return;

            for (auto door : gs->AllDoors)
            {
                if (!door) continue;

                door->bLocked = false;
            }
        }
    };

    static unlock_doors _unlock_doors("unlock_doors", "Unlock All Doors", "");
}