#include "view.hpp"
#include "../core/submenu.hpp"

namespace big
{
    void view::aimbot_submenu()
    {
        canvas::add_tab<regular_submenu>("Aimbot", "SubmenuAimbot"_hash, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("aimbot"_hash);
            sub->add_option<choose_option<std::string, int>>("Aimbot Target", nullptr, &g_settings.aimbot.aimbot_targets, &g_settings.aimbot.aimbot_target);
            sub->add_option<bool_option<bool>>("draw_fov"_hash);
            sub->add_option<number_option<float>>("aimbot_fov"_hash);
            sub->add_option<number_option<float>>("aimbot_smooth"_hash);
            sub->add_option<number_option<int>>("aimbot_trigger"_hash);
            sub->add_option<bool_option<bool>>("triggerbot"_hash);
            sub->add_option<number_option<float>>("trigger_fov"_hash);
        });
    }
}