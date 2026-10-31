#include "../view.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "../core/submenu.hpp"
#include "events/player_event.hpp"
#include <astra/host/input_service.hpp>

#include <astra/components.hpp>
#include <astra/host/canvas.hpp>

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
			sub->add_option<bool_option<bool>>("accept_client_requests"_hash);
			sub->add_option<bool_option<bool>>("career_stats_overlay"_hash);
			sub->add_option<reguler_option>("host_link_ping"_hash);
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
			sub->add_option<reguler_option>("Add Magazine", "A full magazine of the loaded ammo for the gun in their hands (host only)", [] {
				player_event::act(player_action::add_magazine);
			});
			sub->add_option<reguler_option>(player.godmode ? "Godmode: On" : "Godmode: Off", "They take no damage until the next mission (host only)", [] {
				player_event::act(player_action::godmode);
			});
			if (player.local)
			{
				const auto hint = std::string("The name your entry shows in the lobby's player list and in game, on your screen");
				sub->add_option<reguler_option>("Change Name", hint.c_str(), [state = player.state] {
					g_input_service.show("Name to show", [state](std::string const& input) {
						player_event::rename(state, input);
					});
				});
				return;
			}

			const auto other_hint = std::string("Host only: their name, written on the server so every client gets it");
			sub->add_option<reguler_option>("Change Name", other_hint.c_str(), [state = player.state] {
				g_input_service.show("Name to show", [state](std::string const& input) {
					player_event::rename(state, input);
				});
			});

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

	// Every career stat in a panel over the game, the way the mission tracker is drawn, so a stat can be set without
	// walking the menu's lists. The rows come from player_event's copy, so the render thread never touches the engine;
	// a stat the game names a weapon or armour in offers the loadout's own names, taken from the give item panel's lists.
	void view::career_stats_panel()
	{
		if (!_career_stats_overlay.get_state())
			return;

		bool open = true;
		if (canvas::begin_panel("Career Stats", &open, {.size = {600.f, 620.f}}))
		{
			static char search[64]{};
			astra::search_field("career_stats_search", search, sizeof(search), "Search a stat, such as suspects or playtime");

			// What the player typed or picked, kept per stat so it survives the page being drawn again every frame.
			static std::map<std::string, int> numbers;
			static std::map<std::string, std::string> texts;
			static std::map<std::string, int> picks;

			std::vector<stat_info> found;
			for (const auto& stat : player_event::stats())
			{
				if (astra::matches_search(stat.label, search) || astra::matches_search(stat.id, search))
					found.push_back(stat);
			}

			constexpr float field = 190.f;
			// The list sits straight on the panel: no child background, which the menu theme tints blue.
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::BeginChild("career_stats_rows", {0.f, -ImGui::GetTextLineHeightWithSpacing() - 4.f});
			for (const auto& stat : found)
			{
				ImGui::PushID(stat.id.c_str());

				// An average is written as the value times the samples behind it, so it is marked as such.
				const auto label = stat.type == 4 ? std::format("{} (average)", stat.label) : stat.label;
				ImGui::TextUnformatted(label.c_str());
				ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - field - 62.f);
				ImGui::SetNextItemWidth(field);

				if (stat.type == 5 || stat.type == 6 || !stat.numeric)
				{
					auto& text = texts.try_emplace(stat.id, stat.text).first->second;
					const auto choices = player_event::stat_choices(stat.id);
					if (!choices.empty())
					{
						auto& pick = picks.try_emplace(stat.id, 0).first->second;
						for (int i = 0; i < static_cast<int>(choices.size()); i++)
						{
							if (choices[i] == text)
								pick = i;
						}
						if (astra::choice("##pick", &pick, choices))
							text = choices[pick];
					}
					else
					{
						astra::input_text("##text", &text, "text");
					}

					ImGui::SameLine();
					if (astra::small_button("Set"))
						player_event::set_stat(stat, text);
				}
				else
				{
					auto& number = numbers.try_emplace(stat.id, static_cast<int>(stat.value)).first->second;
					astra::input_number("##value", &number);
					ImGui::SameLine();
					if (astra::small_button("Set"))
						player_event::set_stat(stat, std::to_string(number));
				}

				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();
		}
		canvas::end_panel();

		if (!open)
			_career_stats_overlay.set_state(false);
	}
}
