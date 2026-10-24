#include "../view.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "../core/submenu.hpp"

namespace big
{
	void view::online_players_submenu()
	{
		// Rebuilt by the menu on the render thread, so it reads plain fields only and calls nothing in the engine.
		canvas::add_submenu<regular_submenu>("Online Players", "SubmenuPlayerList"_hash, [](regular_submenu* sub)
		{
			auto gs = ron::game_state();
			if (!gs)
				return;

			auto& players = gs->AllPlayerCharacters;

			for (int i = 0; i < players.Num(); i++)
			{
				if (!players.IsValidIndex(i)) continue;

				auto player = players[i];

				if (!player || !player->PlayerState) continue;

				const auto name = player->PlayerState->PlayerNamePrivate.ToString();
				const auto status = player->CharacterHealth ? player->CharacterHealth->HealthStatus : SDK::EPlayerHealthStatus::HS_NotAvailable;

				sub->add_option<reguler_option>(std::format("[{}] {}", i + 1, name).c_str(), ron::health_status_name(status));
			}
		});
	}
}
