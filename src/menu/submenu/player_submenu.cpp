#include "../view.hpp"
#include "utility/player.hpp"
#include "../core/submenu.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Self", SubmenuPlayer, [](regular_submenu* sub)
        {
                sub->add_option<bool_option<bool>>("godmode"_hash);
                sub->add_option<bool_option<bool>>("infinite_ammo"_hash);
                sub->add_option<bool_option<bool>>("damage"_hash);
                sub->add_option<bool_option<bool>>("no_recoil"_hash);
                sub->add_option<bool_option<bool>>("no_spread"_hash);
                sub->add_option<bool_option<bool>>("penetrate_wall"_hash);
                sub->add_option<bool_slider_float_option>("walk_speed"_hash, "walk_mult"_hash);
                sub->add_option<reguler_option>("add_magazine"_hash);
                sub->add_option<reguler_option>("auto_arrest"_hash);
                sub->add_option<reguler_option>("evidance"_hash);
        });
    }
}