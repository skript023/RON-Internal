#include "../view.hpp"
#include "../core/submenu.hpp"
#include "fiber_pool.hpp"
#include "utility/unreal_engine_utility.hpp"

namespace big
{
    void view::setting_submenu()
    {
        // Menu Layout, Theme, Banner, Transparency, colours, Input, Menu controls, and Draw Mouse come from Astra;
        // these rows are what only this game adds.
        canvas::add_settings_tab(SubmenuSettings, SubmenuSettingsInput, [](regular_submenu* sub)
        {
            sub->add_option<sub_option>("Debug", nullptr, "SubmenuDebug"_hash);
            sub->add_option<reguler_option>("Reload Setting", nullptr, [] { g_settings.load(); });
            sub->add_option<reguler_option>("Unload", nullptr, [] { g_running = false; });
            sub->add_option<reguler_option>("Exit", nullptr, [] { exit(0); });
        });

        canvas::add_submenu<regular_submenu>("Debug", "SubmenuDebug"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("Logging Process Event", nullptr, &g_settings.debug.logging_process_event);
            sub->add_option<reguler_option>("Log Player Bones", nullptr, [] {
                g_fiber_pool->queue_job([] {
                    auto character = unreal_engine::get_character();

                    if (!character)
                        return;

                    if (auto mesh = character->Mesh)
                    {
                        for (int i = 0; i < mesh->GetNumBones(); i++)
                        {
                            auto name = mesh->GetBoneName(i);

                            LOG(INFO) << "Bone " << i << ": " << name.ToString();
                        }
                    }
                });
            });
        });
    }
}
