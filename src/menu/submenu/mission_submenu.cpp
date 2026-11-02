#include "../view.hpp"
#include "../core/submenu.hpp"
#include "events/mission_event.hpp"

namespace big
{
    void view::mission_submenu()
    {
        canvas::add_tab<regular_submenu>("Mission", SubmenuMission, [](regular_submenu* sub)
        {
            sub->add_option<bool_option<bool>>("mission_overlay"_hash);
            sub->add_option<bool_option<bool>>("target_inspector"_hash);
            sub->add_option<bool_option<bool>>("collectibles_window"_hash);
            sub->add_option<bool_option<bool>>("objective_alerts"_hash);
            sub->add_option<bool_option<bool>>("no_penalty"_hash);
            sub->add_option<bool_option<bool>>("surrender_on_yell"_hash);

            sub->add_option<reguler_option>("auto_win"_hash);
            sub->add_option<reguler_option>("auto_arrest"_hash);
            sub->add_option<reguler_option>("surrender_suspects"_hash);
            sub->add_option<reguler_option>("disarm_suspects"_hash);
            sub->add_option<reguler_option>("knockout_suspects"_hash);
            sub->add_option<reguler_option>("report_all"_hash);
            sub->add_option<reguler_option>("collect_evidence"_hash);
            sub->add_option<reguler_option>("unlock_doors"_hash);
            sub->add_option<reguler_option>("open_doors"_hash);
            sub->add_option<reguler_option>("disarm_traps"_hash);
            sub->add_option<reguler_option>("remove_wedges"_hash);
            sub->add_option<reguler_option>("teleport_next_ai"_hash);
        });
    }
}
