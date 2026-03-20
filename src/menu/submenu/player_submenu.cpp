#include "../view.hpp"
#include "utility/player.hpp"
#include "../core/submenu.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Player", SubmenuPlayer, [](regular_submenu* sub)
        {
            //sub->add_option<bool_option<bool>>("One Hit Kill", nullptr, &g_settings.self.one_hit);
            /*sub->add_option<bool_option<bool>>("Godmode", "Sometimes you hit doesn't applied to enemy", &g_settings.self.godmode);
            sub->add_option<bool_option<bool>>("zombiemode"_J);
            sub->add_option<bool_option<bool>>("noclip"_J);
            sub->add_option<bool_option<bool>>("autoloot"_J);
            sub->add_option<bool_option<bool>>("nocooldown"_J);
            sub->add_option<bool_option<bool>>("forcecritical"_J);
            sub->add_option<bool_option<bool>>("nofalldmg"_J);
            sub->add_option<bool_option<bool>>("dumbenemy"_J);

            sub->add_option<number_option<int>>("Multihit", "Set to 0 to disable", &g_settings.self.multi_hit, 1, 100);*/
        });
    }
}