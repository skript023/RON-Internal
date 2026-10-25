#include "view.hpp"
#include "../core/submenu.hpp"

namespace big
{
    void view::esp_submenu()
    {
        canvas::add_tab<regular_submenu>("ESP", SubmenuESP, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("esp_activate"_hash);
            sub->add_option<number_option<float>>("esp_distance"_hash, 5.f, 0, true, "", " m");
            sub->add_option<bool_slider_float_option>("radar"_hash, "radar_range"_hash, 5.f, 0);
            sub->add_option<bool_option<bool>>("grenade_path"_hash);

            sub->add_option<bool_option<bool>>("esp_suspects"_hash);
            sub->add_option<bool_option<bool>>("esp_civilians"_hash);
            sub->add_option<bool_option<bool>>("esp_swat"_hash);
            sub->add_option<bool_option<bool>>("esp_players"_hash);
            sub->add_option<bool_option<bool>>("esp_traps"_hash);
            sub->add_option<bool_option<bool>>("esp_doors"_hash);
            sub->add_option<bool_option<bool>>("esp_evidence"_hash);
            sub->add_option<bool_option<bool>>("esp_reportables"_hash);
            sub->add_option<bool_option<bool>>("esp_collectibles"_hash);

            sub->add_option<bool_option<bool>>("draw_name"_hash);
            sub->add_option<bool_option<bool>>("draw_line"_hash);
            sub->add_option<bool_option<bool>>("draw_health"_hash);
            sub->add_option<bool_option<bool>>("draw_box"_hash);
            sub->add_option<bool_option<bool>>("draw_corner_box"_hash);
            sub->add_option<bool_option<bool>>("draw_box_3d"_hash);
            sub->add_option<bool_option<bool>>("draw_skeleton"_hash);
        });
    }
}
