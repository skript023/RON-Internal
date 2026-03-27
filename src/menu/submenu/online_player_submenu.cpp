#include "../view.hpp"
#include "utility/player.hpp"
#include "../core/submenu.hpp"

namespace big
{
	void view::online_players_submenu()
	{
		canvas::add_submenu<regular_submenu>("Online Players", "SubmenuPlayerList"_hash, [](regular_submenu* sub)
		{
			auto world = SDK::UWorld::GetWorld(); if (!world) return;

			if (auto gs = static_cast<SDK::AReadyOrNotGameState*>(world->GameState))
			{
				auto& players = gs->AllPlayerCharacters;

				for (size_t i = 0; i < players.Num(); i++)
				{
					if (!players.IsValidIndex(i)) continue;

					auto player = players[i];

					if (!player || !player->PlayerState) continue;

					auto ps = player->PlayerState;

					sub->add_option<sub_option>(ps->GetPlayerName().ToString().c_str(), nullptr, "SubmenuSelectedPlayer"_hash);
				}
			}
		});
	}
}