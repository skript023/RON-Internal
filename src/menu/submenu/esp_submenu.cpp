#include "../view.hpp"
#include "../core/submenu.hpp"

#include "thread_pool.hpp"
#include "events/entity_event.hpp"
#include "utility/unreal_engine_utility.hpp"

namespace big
{
    void view::esp_submenu()
    {
        canvas::add_tab<regular_submenu>("ESP", SubmenuESP, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("esp_activate"_hash);
            sub->add_option<bool_option<bool>>("draw_team"_hash);
            sub->add_option<bool_option<bool>>("draw_line"_hash);
            sub->add_option<bool_option<bool>>("draw_name"_hash);
            sub->add_option<bool_option<bool>>("draw_skeleton"_hash);
            sub->add_option<bool_option<bool>>("draw_health"_hash);
            sub->add_option<bool_option<bool>>("draw_box"_hash);
            sub->add_option<bool_option<bool>>("godmode"_hash);
            sub->add_option<reguler_option>("Auto Arrest", nullptr, [] {
                g_fiber_pool->queue_job([] {
                    auto world = SDK::UWorld::GetWorld(); if (!world) return;
                    auto level = world->PersistentLevel; if (!level) return;

                    if (auto actors = level->Actors; actors.Num() > 0)

                    for (size_t i = 0; i < actors.Num(); i++)
                    {
                        if (!actors.IsValidIndex(i)) continue;

                        auto actor = actors[i];

                        if (!actor) continue;

                        if (actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
                            if (auto target_pawn = static_cast<SDK::AReadyOrNotCharacter*>(actor))
                                if (target_pawn->DefaultTeam == SDK::ETeamType::TT_CIVILIAN || target_pawn->DefaultTeam == SDK::ETeamType::TT_SUSPECT)
                                {
                                    if (!unreal_engine::get_character()) return;

                                    target_pawn->ArrestComplete(unreal_engine::get_character(), nullptr);
                                    unreal_engine::get_character()->Server_ReportToTOC(target_pawn, false, false);
                                }
                    }
                });
            });
            sub->add_option<reguler_option>("Debug Bone", nullptr, [] {
                g_thread_pool->queue_job([] {
                    auto world = SDK::UWorld::GetWorld(); if (!world) return;
                    auto level = world->PersistentLevel; if (!level) return;

                    if (auto actors = level->Actors; actors.Num() > 0)

                        for (size_t i = 0; i < actors.Num(); i++)
                        {
                            if (!actors.IsValidIndex(i)) continue;

                            auto actor = actors[i];

                            if (!actor) continue;

                            if (actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
                                if (auto target_pawn = static_cast<SDK::AReadyOrNotCharacter*>(actor))
                                    if (auto mesh = target_pawn->Mesh)
                                        for (size_t i = 0; mesh->GetNumBones(); i++)
                                            LOG(INFO) << "BonesIndex: " << i << " Name: " << mesh->GetBoneName(i).ToString();
                        }
                 });
            });
            //sub->add_option<number_option<float>>("Max Draw Distance", "ESP must be enabled", &g_settings.esp.global_render_distance[1], 0.f, 1000.f, 1.f, 1);
        });
    }
}