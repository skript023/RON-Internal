#include "../view.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "../core/submenu.hpp"
#include "events/player_event.hpp"

namespace big
{
	static const char* status_text(const player_info& player)
	{
		return player.has_character ? ron::health_status_name(player.status) : "No character";
	}

	void view::online_players_submenu()
	{
		// Both pages are rebuilt by the menu on the render thread, so they draw from player_event's copy and call nothing
		// in the engine.
		canvas::add_submenu<regular_submenu>("Online Players", "SubmenuPlayerList"_hash, [](regular_submenu* sub)
		{
			sub->add_option<bool_option<bool>>("player_alerts"_hash);
			if (player_event::is_spectating())
				sub->add_option<reguler_option>("Stop Spectating", "Brings the camera back to you", [] { player_event::stop_spectating(); });

			const auto players = player_event::players();
			for (std::size_t i = 0; i < players.size(); i++)
			{
				const auto& player = players[i];
				const auto label = std::format("[{}] {}{}", i + 1, player.name, player.local ? " (you)" : "");
				const auto description = player.distance >= 0.f
				    ? std::format("{}, {:.0f} m away", status_text(player), player.distance)
				    : std::string(status_text(player));

				sub->add_option<sub_option>(label.c_str(), description.c_str(), "SubmenuSelectedPlayer"_hash, [state = player.state] {
					player_event::select(state);
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Player", "SubmenuSelectedPlayer"_hash, [](regular_submenu* sub)
		{
			const auto selected = player_event::selected();
			if (!selected)
			{
				sub->set_name("Player");
				sub->add_option<reguler_option>("This player has left the lobby.");
				return;
			}

			const auto& player = *selected;
			sub->set_name(player.name.c_str());

			auto info = [sub](const std::string& text) {
				sub->add_option<reguler_option>(text.c_str());
			};

			if (player.local || player.host)
				info(std::format("Role: {}{}", player.host ? "Host" : "Player", player.local ? " (you)" : ""));
			info(std::format("Status: {}", status_text(player)));
			if (player.has_character)
			{
				info(std::format("Health: {:.0f} / {:.0f}", player.health, player.max_health));
				info(std::format("Head: {:.0f}%", player.head));
				info(std::format("Arms: L {:.0f}%  R {:.0f}%", player.left_arm, player.right_arm));
				info(std::format("Legs: L {:.0f}%  R {:.0f}%", player.left_leg, player.right_leg));
				if (!player.item.empty())
					info(std::format("Holding: {}", player.item));
				if (player.ammo >= 0)
					info(std::format("Ammo: {} / {}, {} magazines", player.ammo, player.capacity, player.magazines));
				if (player.distance >= 0.f)
					info(std::format("Distance: {:.0f} m", player.distance));
			}
			if (!player.local)
				info(std::format("Ping: {} ms", player.ping));
			info(std::format("Platform: {}", player.platform.empty() ? "Unknown" : player.platform));
			if (!player.id.empty())
				info(std::format("ID: {}", player.id));

			if (!player.local)
			{
				sub->add_option<reguler_option>("Teleport to Player", "Puts you right behind them", [] {
					player_event::act(player_action::teleport);
				});
				sub->add_option<reguler_option>(player.spectated ? "Stop Spectating" : "Spectate", "Your camera follows them, on your screen only", [] {
					player_event::act(player_action::spectate);
				});
			}
			sub->add_option<reguler_option>("Heal & Revive", "Full health on every limb, and back on their feet when down (host only)", [] {
				player_event::act(player_action::heal);
			});
			sub->add_option<reguler_option>("Refill Ammo", "A full magazine for the gun in their hands (host only)", [] {
				player_event::act(player_action::refill_ammo);
			});
			sub->add_option<reguler_option>(player.godmode ? "Godmode: On" : "Godmode: Off", "They take no damage until the next mission (host only)", [] {
				player_event::act(player_action::godmode);
			});
			if (player.local)
				return;

			sub->add_option<reguler_option>(player.muted ? "Unmute" : "Mute", "Their voice chat, for you only", [] {
				player_event::act(player_action::mute);
			});

			// A kick cannot be undone, so it takes a second click within a few seconds.
			static weak_object<SDK::APlayerState> kick_target;
			static auto kick_armed = std::chrono::steady_clock::time_point{};
			const bool armed = kick_target == player.state && std::chrono::steady_clock::now() - kick_armed < 5s;
			sub->add_option<reguler_option>(armed ? "Kick: Click Again to Confirm" : "Kick", "Removes them from the lobby (host only)", [armed, state = player.state] {
				if (armed)
				{
					kick_target = {};
					player_event::act(player_action::kick);
				}
				else
				{
					kick_target = state;
					kick_armed = std::chrono::steady_clock::now();
				}
			});
		});
	}
}
