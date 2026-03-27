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
    class auto_win : public command
    {
        using command::command;

        void on_call() override
        {
            auto world = SDK::UWorld::GetWorld(); if (!world) return;
            auto level = world->PersistentLevel; if (!level) return;
            auto game_state = static_cast<SDK::AReadyOrNotGameState*>(world->GameState);

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
                    {
                        if (auto target_pawn = static_cast<SDK::AReadyOrNotCharacter*>(actor))
                        {
                            player = target_pawn;
                            if (target_pawn->DefaultTeam == SDK::ETeamType::TT_CIVILIAN || target_pawn->DefaultTeam == SDK::ETeamType::TT_SUSPECT)
                            {
                                if (!unreal_engine::get_character())
                                {
                                    LOG(FATAL) << "Player character is null.";

                                    return;
                                }

                                target_pawn->ArrestComplete(unreal_engine::get_character(), nullptr);
                                unreal_engine::get_character()->Server_ReportToTOC(target_pawn, false, false);
                            }
                        }
                    }

                    if (actor->IsA(SDK::ABaseWeapon::StaticClass()))
                    {
                        SDK::ABaseWeapon* Weapon = reinterpret_cast<SDK::ABaseWeapon*>(actor);

                        // Verify both weapon and component are valid
                        if (Weapon && Weapon->EvidenceComponent && Weapon->EvidenceComponent->CanBeCollected())
                        {
                            weapons.Add(Weapon);
                        }
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

            for (SDK::AEvidenceActor* Evidence : game_state->AllEvidenceActors)
            {
                if (!Evidence) continue;
                if (Evidence->EvidenceComponent && Evidence->EvidenceComponent->CanBeCollected())
                {
                    for (SDK::FScoreBonus& Bonus : Evidence->ScoringComponent->ScoringData.Bonuses)
                    {
                        Bonus.bEnabled = true;
                        Bonus.bGiven = true;
                        Bonus.Score = 10000;
                    }
                    for (SDK::FScorePenalty& Penalty : Evidence->ScoringComponent->ScoringData.Penalties)
                    {
                        Penalty.bEnabled = false;
                        Penalty.bGiven = false;
                        Penalty.Score = 0;
                    }
                    Evidence->OnEvidenceStateChanged(SDK::EEvidenceActorState::Collected);
                }
            }

            for (SDK::AReportableActor* Actor : game_state->AllReportableActors)
            {
                if (auto chara = unreal_engine::get_character())
                {
                    chara->Server_ReportToTOC(Actor, false, false);
                    chara->Server_ReportTarget(Actor);
                    Actor->InteractableComponent->OnInteract(((SDK::APlayerCharacter*)chara));
                }
            }

            for (SDK::AObjective* objective : game_state->MissionObjectives)
            {
                if (!objective) continue;
                for (SDK::FScoreBonus& bonus : objective->ScoringComponent->ScoringData.Bonuses)
                {
                    bonus.bEnabled = true;
                    bonus.bGiven = true;
                    bonus.Score = 10000;

                }
                for (SDK::FScorePenalty& penalty : objective->ScoringComponent->ScoringData.Penalties)
                {
                    penalty.bEnabled = false;
                    penalty.bGiven = false;
                    penalty.Score = 0;
                }
            }
        }
    };

    static auto_win _auto_win("auto_win", "Auto Win", "");
}