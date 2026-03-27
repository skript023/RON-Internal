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
    class evidance : public command
    {
        using command::command;

        void on_call() override
        {
            auto world = SDK::UWorld::GetWorld(); if (!world) return;
            auto level = world->PersistentLevel; if (!level) return;

            SDK::AReadyOrNotCharacter* player = nullptr;

            SDK::TAllocatedArray<SDK::ABaseWeapon*> weapons(40);

            if (auto actors = level->Actors; actors.Num() > 0)
            {
                for (size_t i = 0; i < actors.Num(); i++)
                {
                    if (!actors.IsValidIndex(i)) continue;

                    auto actor = actors[i];

                    if (!actor) continue;

                    if (actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
                        player = static_cast<SDK::AReadyOrNotCharacter*>(actor);

                    if (actor->IsA(SDK::ABaseWeapon::StaticClass()))
                    {
                        auto weapon = reinterpret_cast<SDK::ABaseWeapon*>(actor);

                        // Verify both weapon and component are valid
                        if (weapon && weapon->EvidenceComponent && weapon->EvidenceComponent->CanBeCollected())
                        {
                            weapons.Add(weapon);
                        }
                    }
                }

                for (int i = 0; i < weapons.Num(); i++)
                {
                    if (!weapons[i]) continue;
                    if (auto c = unreal_engine::get_character())
                        c->PickupEvidence(weapons[i]);
                    else if (player)
                        player->PickupEvidence(weapons[i]);
                }
            }
        }
    };

    static evidance _evidance("evidance", "Collect All Evidance", "");
}