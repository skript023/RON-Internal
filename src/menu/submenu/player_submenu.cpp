#include "../view.hpp"
#include "utility/player.hpp"
#include "../core/submenu.hpp"
// #include "features/visual_filter.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Self", SubmenuPlayer, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("godmode"_hash);
            sub->add_option<bool_option<bool>>("no_stun"_hash);
            sub->add_option<bool_option<bool>>("no_suppression"_hash);
            sub->add_option<bool_option<bool>>("no_hit_effects"_hash);
            sub->add_option<bool_slider_float_option>("bright_vision"_hash, "bright_vision_level"_hash, 0.5f);
            // sub->add_option<sub_option>("Visual Filter", "Lays one of the game's post-process materials over your screen", "SubmenuVisualFilter"_hash);
            sub->add_option<bool_option<bool>>("infinite_ammo"_hash);
            sub->add_option<reguler_option>("add_magazine"_hash);
            sub->add_option<bool_option<bool>>("infinite_tactical"_hash);
            sub->add_option<bool_option<bool>>("give_item_window"_hash);
            sub->add_option<bool_option<bool>>("damage"_hash);
            sub->add_option<bool_option<bool>>("no_recoil"_hash);
            sub->add_option<bool_option<bool>>("no_spread"_hash);
            sub->add_option<bool_slider_float_option>("rapid_fire"_hash, "fire_rate_mult"_hash, 0.5f);
            sub->add_option<bool_option<bool>>("full_auto"_hash);
            sub->add_option<bool_slider_float_option>("penetrate_wall"_hash, "penetration_mult"_hash, 1.f);
            sub->add_option<bool_option<bool>>("instant_lockpick"_hash);
            sub->add_option<bool_option<bool>>("instant_cuff"_hash);
            sub->add_option<bool_slider_float_option>("fast_hands"_hash, "hands_speed"_hash, 0.5f);
            sub->add_option<bool_slider_float_option>("walk_speed"_hash, "walk_mult"_hash, 0.1f);
            sub->add_option<bool_slider_float_option>("noclip"_hash, "noclip_speed"_hash, 1.f);
            sub->add_option<bool_option<bool>>("third_person"_hash);
            sub->add_option<sub_option>("Online Players", nullptr, "SubmenuPlayerList"_hash);
        });

        // Left out of the menu for now; features/visual_filter works. Rebuilt by the menu on the render thread, so it draws
        // from visual_filter's copy.
        // canvas::add_submenu<regular_submenu>("Visual Filter", "SubmenuVisualFilter"_hash, [](regular_submenu* sub)
        // {
        //     sub->add_option<reguler_option>("Turn Off", nullptr, [] { visual_filter::turn_off(); });
        //     sub->add_option<reguler_option>("Find Filters", "Reads the list again; the game loads more materials as it goes", [] { visual_filter::find(); });
        //     for (const auto& filter : visual_filter::list())
        //     {
        //         sub->add_option<reguler_option>(filter.name.c_str(), nullptr, [name = filter.name] {
        //             visual_filter::toggle(name);
        //         }).set_status(filter.on ? astra::status_kind::done : astra::status_kind::none, filter.on ? "On" : nullptr);
        //     }
        // });
    }
}
