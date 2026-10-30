#include "../view.hpp"
#include "../core/submenu.hpp"

namespace big
{
    void view::world_submenu()
    {
        canvas::add_tab<regular_submenu>("World", SubmenuWorld, [](regular_submenu* sub)
        {
            sub->add_option<bool_slider_float_option>("slow_motion"_hash, "time_scale"_hash, 0.05f, 2);
            sub->add_option<bool_slider_float_option>("damage_multiplier"_hash, "damage_scale"_hash, 0.25f, 2);
            sub->add_option<bool_option<bool>>("freeze_ai"_hash);
            sub->add_option<bool_option<bool>>("ai_ignore"_hash);
            sub->add_option<bool_option<bool>>("one_kick_doors"_hash);
            sub->add_option<bool_option<bool>>("objective_markers"_hash);
            sub->add_option<bool_option<bool>>("evidence_markers"_hash);
            sub->add_option<bool_option<bool>>("infinite_swat_items"_hash);
            sub->add_option<reguler_option>("reset_roster_stress"_hash);
        });
    }
}
