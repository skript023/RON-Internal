#include "player_event.hpp"
#include "script.hpp"
#include "task_mgr.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "features/host_link.hpp"

#include <astra/host/notification.hpp>

namespace big
{
	namespace
	{
		using clock = std::chrono::steady_clock;

		// What the render thread may read. Everything else here belongs to the game thread.
		struct shared_state
		{
			std::mutex mutex;
			std::vector<player_info> players;
			weak_object<SDK::APlayerState> selected;
			bool spectating = false;
		};

		shared_state g_shared;

		std::atomic<clock::time_point> g_detail_seen{};

		bool g_spectating = false;
		weak_object<SDK::APlayerCharacter> g_spectated;

		struct tracked_player
		{
			weak_object<SDK::APlayerState> state;
			std::string name;
		};

		struct tracked_status
		{
			weak_object<SDK::APlayerState> state;
			SDK::EPlayerHealthStatus status;
		};

		std::vector<tracked_player> g_lobby;
		std::vector<tracked_status> g_statuses;
		const void* g_lobby_game_state = nullptr;
		clock::time_point g_lobby_since{};

		weak_object<SDK::UPlayersList> g_players_list;

		const char* platform_name(SDK::EGamePlatform platform)
		{
			switch (platform)
			{
			case SDK::EGamePlatform::Steam: return "Steam";
			case SDK::EGamePlatform::SteamDeck: return "Steam Deck";
			case SDK::EGamePlatform::GeforceNow: return "GeForce Now";
			case SDK::EGamePlatform::Xbox: return "Xbox";
			case SDK::EGamePlatform::Playstation: return "PlayStation";
			case SDK::EGamePlatform::EGS: return "Epic Games";
			case SDK::EGamePlatform::XboxPC: return "Xbox PC";
			default: return "Unknown";
			}
		}

		float limb_percent(const SDK::FLimbHealthData& limb)
		{
			return limb.MaxHealth > 0.f ? limb.Health / limb.MaxHealth * 100.f : 0.f;
		}

		SDK::UPlayersList* players_list(bool force)
		{
			if (auto list = g_players_list.get())
				return list;

			static auto last_search = clock::time_point{};
			if (!force && clock::now() - last_search < 5s)
				return nullptr;
			last_search = clock::now();

			const auto cls = SDK::UPlayersList::StaticClass();
			for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
			{
				auto object = SDK::UObject::GObjects->GetByIndex(i);
				if (!object || object->IsDefaultObject() || !object->IsA(cls))
					continue;

				g_players_list = static_cast<SDK::UPlayersList*>(object);
				return g_players_list.get();
			}

			return nullptr;
		}

		const SDK::FLobbyPlayer* lobby_entry(SDK::UPlayersList* list, const std::string& name)
		{
			for (const auto& entry : list->PlayersList)
			{
				if (entry.DisplayName.ToString() == name || entry.RealName.ToString() == name)
					return &entry;
			}

			return nullptr;
		}

		SDK::APlayerCharacter* character_of(SDK::APlayerState* state)
		{
			if (auto gs = ron::game_state())
			{
				for (auto character : gs->AllPlayerCharacters)
				{
					if (unreal_engine::is_valid_actor(character) && character->PlayerState == state)
						return character;
				}
			}

			auto pawn = state->PawnPrivate;
			if (unreal_engine::is_valid_actor(pawn) && pawn->IsA(SDK::APlayerCharacter::StaticClass()))
				return static_cast<SDK::APlayerCharacter*>(pawn);

			return nullptr;
		}

		player_info read_player(SDK::APlayerState* state, SDK::APlayerController* controller, SDK::UPlayersList* list, const std::optional<SDK::FVector>& origin)
		{
			player_info info{};
			info.state = state;
			info.name = state->PlayerNamePrivate.ToString();
			info.local = state == controller->PlayerState;
			info.ping = state->CompressedPing * 4;

			if (state->IsA(SDK::AReadyOrNotPlayerState::StaticClass()))
			{
				auto ron_state = static_cast<SDK::AReadyOrNotPlayerState*>(state);
				info.host = ron_state->bHosting;
				info.platform = platform_name(ron_state->GamePlatform);
			}

			if (list)
			{
				if (auto entry = lobby_entry(list, info.name))
				{
					info.id = entry->UniqueNetId.ToString();
					info.muted = list->GetMutedState(entry->UniqueNetId);
				}
			}

			auto character = character_of(state);
			if (!character)
				return info;

			info.has_character = true;
			info.spectated = g_spectating && g_spectated.get() == character;
			info.godmode = character->bGodMode;
			info.health = character->GetCurrentHealth();
			info.max_health = character->GetMaxHealth();
			if (!info.local && origin)
				info.distance = static_cast<float>(origin->GetDistanceToInMeters(character->K2_GetActorLocation()));

			if (auto health = character->CharacterHealth)
			{
				info.status = health->HealthStatus;
				info.head = limb_percent(health->Head);
				info.left_arm = limb_percent(health->LeftArm);
				info.right_arm = limb_percent(health->RightArm);
				info.left_leg = limb_percent(health->LeftLeg);
				info.right_leg = limb_percent(health->RightLeg);
			}

			if (auto item = character->GetEquippedItem())
			{
				info.item = ron::text(item->ItemName);
				if (item->IsA(SDK::ABaseMagazineWeapon::StaticClass()))
				{
					auto weapon = static_cast<SDK::ABaseMagazineWeapon*>(item);
					info.ammo = weapon->GetCurrentMagazine().Ammo;
					info.capacity = weapon->Capacity;
					info.magazines = weapon->Magazines.Num();
				}
			}

			return info;
		}

		// Players already here when a map loads, or still loading into it, are not announced as joining.
		void check_lobby(SDK::AReadyOrNotGameState* gs, const std::vector<player_info>& players)
		{
			if (g_lobby_game_state != gs)
			{
				g_lobby.clear();
				g_statuses.clear();
				g_lobby_game_state = gs;
				g_lobby_since = clock::now();
			}

			const bool quiet = clock::now() - g_lobby_since < 15s || !_player_alerts.get_state();

			for (const auto& player : players)
			{
				// The name arrives a moment after the player does.
				if (player.name.empty() || std::ranges::any_of(g_lobby, [&](const tracked_player& t) { return t.state == player.state; }))
					continue;

				g_lobby.push_back({ player.state, player.name });
				if (!quiet && !player.local)
					notification::info("Player joined", player.name);
			}

			std::erase_if(g_lobby, [&](const tracked_player& tracked) {
				if (std::ranges::any_of(players, [&](const player_info& p) { return p.state == tracked.state; }))
					return false;

				if (!quiet)
					notification::info("Player left", tracked.name);
				return true;
			});
		}

		void check_downed(const std::vector<player_info>& players)
		{
			std::erase_if(g_statuses, [&](const tracked_status& tracked) {
				return std::ranges::none_of(players, [&](const player_info& p) { return p.state == tracked.state; });
			});

			for (const auto& player : players)
			{
				if (player.local || !player.has_character)
					continue;

				auto it = std::ranges::find_if(g_statuses, [&](const tracked_status& t) { return t.state == player.state; });
				if (it == g_statuses.end())
				{
					g_statuses.push_back({ player.state, player.status });
					continue;
				}

				if (it->status == player.status)
					continue;

				it->status = player.status;
				const bool down = player.status == SDK::EPlayerHealthStatus::HS_Downed || player.status == SDK::EPlayerHealthStatus::HS_Incapacitated;
				if (!down || !_player_alerts.get_state())
					continue;

				const auto status = ron::health_status_name(player.status);
				notification::warning(std::format("{} is down", player.name),
				    player.distance >= 0.f ? std::format("{}, {:.0f} m away.", status, player.distance) : std::format("{}.", status));
			}
		}

		void view_own_character()
		{
			g_spectating = false;
			g_spectated = {};

			auto controller = unreal_engine::get_player_controller();
			auto character = unreal_engine::get_character();
			if (controller && character)
				controller->SetViewTargetWithBlend(character, 0.3f, SDK::EViewTargetBlendFunction::VTBlend_Cubic, 0.f, false);
		}

		void tick()
		{
			static auto last_read = clock::time_point{};
			if (clock::now() - last_read < 250ms)
				return;
			last_read = clock::now();

			// The camera goes back to you once the player you watch leaves, or their character is gone.
			if (g_spectating && !unreal_engine::is_valid_actor(g_spectated.get()))
				view_own_character();

			auto gs = ron::game_state();
			auto controller = unreal_engine::get_player_controller();
			if (!gs || !controller)
			{
				std::lock_guard lock(g_shared.mutex);
				g_shared.players.clear();
				g_shared.spectating = g_spectating;
				return;
			}

			auto list = clock::now() - g_detail_seen.load() < 1s ? players_list(false) : nullptr;

			std::optional<SDK::FVector> origin;
			if (unreal_engine::get_character())
				origin = player::get_player_coords();

			std::vector<player_info> players;
			for (auto state : gs->PlayerArray)
			{
				if (unreal_engine::is_valid_actor(state))
					players.push_back(read_player(state, controller, list, origin));
			}

			check_lobby(gs, players);
			check_downed(players);

			std::lock_guard lock(g_shared.mutex);
			g_shared.players = std::move(players);
			g_shared.spectating = g_spectating;
		}

		// A fault reading the lobby skips this read instead of ending the worker.
		void tick_guarded()
		{
			TRY_CLAUSE
			{
				tick();
			}
			EXCEPT_CLAUSE
		}

		void heal(SDK::APlayerCharacter* target, const std::string& name)
		{
			auto health = target->CharacterHealth;
			if (!health)
				return;

			using enum SDK::EPlayerHealthStatus;
			if (health->HealthStatus == HS_Dead || health->HealthStatus == HS_Arrested)
			{
				notification::warning("Heal & Revive", std::format("{} is {} and cannot be revived.", name, ron::health_status_name(health->HealthStatus)));
				return;
			}

			// The game's own revive, as if you had finished reviving them.
			if (health->HealthStatus == HS_Downed || health->HealthStatus == HS_Incapacitated)
			{
				if (auto self = unreal_engine::get_character(); self && self != target)
					self->Server_OnReviveComplete(target);
			}

			health->ResetAllLimbHealth();
			target->ResetHealth();
			if (health->HealthStatus != HS_Healthy)
				health->SetHealthStatus(HS_Healthy);

			notification::success("Heal & Revive", std::format("{} is back to full health.", name));
		}

		void refill_ammo(SDK::APlayerCharacter* target, const std::string& name)
		{
			auto weapon = target->GetEquippedWeapon();
			if (!weapon || weapon->Capacity <= 0)
			{
				notification::warning("Refill Ammo", std::format("{} is not holding a gun that takes magazines.", name));
				return;
			}

			SDK::FMagazine magazine{};
			magazine.Ammo = static_cast<UC::uint16>((std::min)(weapon->Capacity, 0xFFFF));
			magazine.AmmoType = weapon->GetCurrentMagazine().AmmoType;
			weapon->Server_AddMagazine(magazine);

			notification::success("Refill Ammo", std::format("Gave {} a magazine of {} rounds.", name, magazine.Ammo));
		}

		void mute(const std::string& name)
		{
			auto list = players_list(true);
			auto entry = list ? lobby_entry(list, name) : nullptr;
			if (!entry)
			{
				notification::warning("Mute", "Open the game's player list once, such as from the pause menu, then try again.");
				return;
			}

			const auto id_text = entry->UniqueNetId.ToWString();
			const SDK::FString id(id_text.c_str());
			if (!list->CanChangeMutedState(id))
			{
				notification::warning("Mute", std::format("The game does not let you mute {}.", name));
				return;
			}

			const bool muted = !list->GetMutedState(id);
			list->SetMutedState(id, muted);
			notification::success("Mute", std::format("{} {}.", name, muted ? "muted" : "unmuted"));
		}

		void kick(SDK::APlayerState* state, const std::string& name)
		{
			auto world = SDK::UWorld::GetWorld();
			auto mode = world ? world->AuthorityGameMode : nullptr;
			auto owner = state->Owner;
			if (!mode || !mode->IsA(SDK::AReadyOrNotGameMode::StaticClass()) || !owner || !owner->IsA(SDK::APlayerController::StaticClass()))
			{
				notification::warning("Kick", std::format("Could not find {}'s connection.", name));
				return;
			}

			const auto reason = SDK::UKismetTextLibrary::Conv_StringToText(SDK::FString(L"Kicked by the host"));
			if (static_cast<SDK::AReadyOrNotGameMode*>(mode)->KickPlayer(static_cast<SDK::APlayerController*>(owner), reason))
				notification::success("Kick", std::format("{} was kicked.", name));
			else
				notification::warning("Kick", std::format("The game refused to kick {}.", name));
		}

		void spectate(SDK::APlayerCharacter* target)
		{
			auto controller = unreal_engine::get_player_controller();
			if (g_spectating && g_spectated.get() == target)
			{
				view_own_character();
			}
			else if (controller)
			{
				controller->SetViewTargetWithBlend(target, 0.3f, SDK::EViewTargetBlendFunction::VTBlend_Cubic, 0.f, false);
				g_spectating = true;
				g_spectated = target;
			}
		}

		// The menu leaves out what makes no sense on yourself: teleport, spectate, mute, and kick.
		void run_action(player_action action, SDK::APlayerState* state)
		{
			const auto name = state->PlayerNamePrivate.ToString();
			if (action == player_action::mute)
			{
				mute(name);
				return;
			}
			if (action == player_action::kick)
			{
				if (ron::require_host("Kick"))
					kick(state, name);
				return;
			}

			auto target = character_of(state);
			if (!target)
			{
				notification::warning("Online Players", std::format("{} has no character right now.", name));
				return;
			}

			// As a client, the host does these when it runs this mod; it finds the player by name.
			if (!ron::is_host() && (action == player_action::heal || action == player_action::refill_ammo || action == player_action::godmode))
			{
				host_link::run("player_action", "Online Players", std::format("{}:{}", static_cast<int>(action), name));
				return;
			}

			switch (action)
			{
			case player_action::teleport:
			{
				// Behind them, so you do not land inside them.
				auto location = target->K2_GetActorLocation();
				const auto forward = target->GetActorForwardVector();
				location.X -= forward.X * 100.f;
				location.Y -= forward.Y * 100.f;
				player::set_player_coords(location, target->K2_GetActorRotation());
				notification::success("Teleport", std::format("Teleported to {}.", name));
				break;
			}
			case player_action::spectate:
				spectate(target);
				break;
			case player_action::heal:
				if (ron::require_host("Heal & Revive"))
					heal(target, name);
				break;
			case player_action::refill_ammo:
				if (ron::require_host("Refill Ammo"))
					refill_ammo(target, name);
				break;
			case player_action::godmode:
				// The host decides damage, so its flag alone protects them.
				if (ron::require_host("Godmode"))
				{
					target->bGodMode = !target->bGodMode;
					notification::success("Godmode", std::format("Godmode {} for {}.", target->bGodMode ? "on" : "off", name));
				}
				break;
			default:
				break;
			}
		}

		// As the host: Heal & Revive, Refill Ammo or Godmode a client asked for, on the player it named.
		host_link::client_action _player_action_for_clients("player_action", "Online Players", [](SDK::APlayerCharacter*, const std::string& args) {
			const auto colon = args.find(':');
			auto gs = ron::game_state();
			if (colon == std::string::npos || !gs)
				return false;

			const auto action = static_cast<player_action>(std::atoi(args.substr(0, colon).c_str()));
			if (action != player_action::heal && action != player_action::refill_ammo && action != player_action::godmode)
				return false;

			const auto name = args.substr(colon + 1);
			for (auto state : gs->PlayerArray)
			{
				if (unreal_engine::is_valid_actor(state) && state->PlayerNamePrivate.ToString() == name)
				{
					run_action(action, state);
					return true;
				}
			}
			return false;
		});
	}

	task<void> player_event::run()
	{
		LOG(INFO) << "Player Event Registered";

		while (g_running)
		{
			tick_guarded();
			co_await this_task::wait();
		}

		if (g_spectating)
			view_own_character();

		co_return;
	}

	std::vector<player_info> player_event::players()
	{
		std::lock_guard lock(g_shared.mutex);
		return g_shared.players;
	}

	bool player_event::is_spectating()
	{
		std::lock_guard lock(g_shared.mutex);
		return g_shared.spectating;
	}

	void player_event::select(weak_object<SDK::APlayerState> state)
	{
		std::lock_guard lock(g_shared.mutex);
		g_shared.selected = state;
	}

	std::optional<player_info> player_event::selected()
	{
		g_detail_seen = clock::now();

		std::lock_guard lock(g_shared.mutex);
		auto it = std::ranges::find_if(g_shared.players, [](const player_info& p) { return p.state == g_shared.selected; });
		if (it == g_shared.players.end())
			return std::nullopt;
		return *it;
	}

	void player_event::act(player_action action)
	{
		weak_object<SDK::APlayerState> state;
		{
			std::lock_guard lock(g_shared.mutex);
			state = g_shared.selected;
		}

		async_run([action, state] {
			if (auto target = state.get())
				run_action(action, target);
			else
				notification::warning("Online Players", "That player has left.");
		});
	}

	void player_event::stop_spectating()
	{
		async_run([] {
			if (g_spectating)
				view_own_character();
		});
	}
}
