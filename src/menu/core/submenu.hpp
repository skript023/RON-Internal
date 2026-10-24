#pragma once

#include <cstdint>
#include <astra/host/canvas.hpp>
#include <astra/host/menu/tabs_menu.hpp>
#include <astra/host/menu/bool_option.hpp>
#include <astra/host/menu/choose_option.hpp>
#include <astra/host/menu/number_option.hpp>
#include <astra/host/menu/reguler_option.hpp>
#include <astra/host/menu/sub_option.hpp>
#include <astra/host/menu/reguler_submenu.hpp>
#include <astra/host/menu/player_submenu.hpp>
#include <astra/host/menu/bool_slider_int_option.hpp>
#include <astra/host/menu/bool_slider_float_option.hpp>

namespace big
{
    enum Submenu : std::uint32_t
	{
		SubmenuHome,
		SubmenuPlayer,
		SubmenuStats,
		SubmenuESP,
		SubmenuMovement,
		SubmenuTeleport,
		SubmenuCustomTeleport,
		SubmenuTest,
		SubmenuPlayerList,
		SubmenuSettings,
		SubmenuSettingsSubmenuBar,
		SubmenuSettingsOption,
		SubmenuSettingsFooter,
		SubmenuSettingsInput,
		SubmenuSelectedPlayer,
		SubmenuMission,
		SubmenuWorld
	};
}
