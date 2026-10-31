#include "player_event.hpp"
#include "script.hpp"
#include "task_mgr.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "features/host_link.hpp"
#include "SDK/W_PlayersList_Entry_classes.hpp"
#include "SDK/UMG_classes.hpp"
#include "SDK/CommonUI_classes.hpp"
#include "SDK/GameplayTags_classes.hpp"

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

		// The game takes your name from your player state and the lobby list rebuilds its rows from it, so a name
		// picked here goes to both: the state through the game's own server call, and the text of a row that is still
		// showing the old name.
		std::string g_spoof_name;
		std::string g_real_name;
		clock::time_point g_spoof_sent{};
		int g_spoof_attempts = 0;
		weak_object<SDK::APlayerState> g_spoof_state; // the state the tries were made on, so a new lobby starts over

		// The online platform type the list compares each player against the viewer's own. Steam's is 4000, so any
		// other number renders the row as another platform. It follows the name: a spoofed name comes with a non-Steam
		// badge, and putting the name back puts 4000 back.
		int g_platform_type = 4000;
		bool g_platform_type_set = false;
		int g_platform_attempts = 0;
		clock::time_point g_platform_sent{};

		std::wstring widen(std::string_view text)
		{
			return std::wstring(text.begin(), text.end());
		}

		SDK::APlayerState* own_state()
		{
			auto controller = unreal_engine::get_player_controller();
			return controller ? controller->PlayerState : nullptr;
		}

		void ask_server_for_name(SDK::APlayerState* state)
		{
			if (!state->IsA(SDK::AReadyOrNotPlayerState::StaticClass()))
				return;

			// Authority here means the call runs on this machine instead of reaching the server.
			const auto role = state->GetLocalRole();
			LOG(INFO) << "Change Name: sending " << g_spoof_name << " with role " << static_cast<int>(role)
			          << (role == SDK::ENetRole::ROLE_Authority ? " (local)" : " (to the server)");

			const auto wide = widen(g_spoof_name);
			static_cast<SDK::AReadyOrNotPlayerState*>(state)->Server_SetPlayerName(SDK::BasicFilesImpleUtils::StringToName(wide.c_str()));

			g_spoof_sent = clock::now();
			++g_spoof_attempts;
		}

		void apply_platform_fields()
		{
			auto state = own_state();
			if (!state || !state->IsA(SDK::AReadyOrNotPlayerState::StaticClass()))
				return;

			auto ron_state = static_cast<SDK::AReadyOrNotPlayerState*>(state);
			g_platform_sent = clock::now();
			++g_platform_attempts;

			// The host is the server and writes the field itself; a client's call for it is validated against the
			// platform it really connected from, so that one only lands when the values match.
			if (ron::is_host())
			{
				ron_state->OnlinePlatformType = g_platform_type;
				return;
			}

			if (auto controller = ron::controller())
				controller->Server_SetPlatformTypes(g_platform_type, ron_state->GamePlatform);
		}

		void keep_platform()
		{
			if (!g_platform_type_set)
				return;

			auto state = own_state();
			if (!state || !state->IsA(SDK::AReadyOrNotPlayerState::StaticClass()))
				return;

			if (static_cast<SDK::AReadyOrNotPlayerState*>(state)->OnlinePlatformType == g_platform_type)
			{
				g_platform_attempts = 0;
				return;
			}

			if (!ron::is_host() && g_platform_attempts >= 3)
				return;

			if (clock::now() - g_platform_sent < 3s)
				return;

			apply_platform_fields();
		}

		// The server answers the call above a moment later, so a name that has not come back yet is not asked for
		// again, and the asking stops after a few tries instead of running for the whole match. A new lobby brings a
		// new player state, which starts the asking over.
		void keep_name()
		{
			if (g_spoof_name.empty())
				return;

			auto state = own_state();
			if (!state)
				return;

			if (g_spoof_state != state)
			{
				g_spoof_state = state;
				g_spoof_attempts = 0;
			}

			if (state->PlayerNamePrivate.ToString() == g_spoof_name)
			{
				// It took, so a name the game resets later can be asked for again.
				g_spoof_attempts = 0;
				return;
			}

			if (g_spoof_attempts >= 3)
				return;

			if (clock::now() - g_spoof_sent < 3s)
				return;

			ask_server_for_name(state);
		}

		// TextBlock.SetText, reached by name: the SDK only compiles the game's own function files, so UMG's call
		// cannot be linked against.
		void set_text(SDK::UObject* block, const std::string& text)
		{
			auto function = block->Class->GetFunction("TextBlock", "SetText");
			if (!function)
				return;

			const auto wide = widen(text);
			const SDK::FString value(wide.c_str());
			struct
			{
				SDK::FText InText;
			} args{ SDK::UKismetTextLibrary::Conv_StringToText(value) };

			const auto flags = function->FunctionFlags;
			function->FunctionFlags |= 0x400; // FUNC_Native, the flag the SDK sets before it calls a game function
			block->ProcessEvent(function, &args);
			function->FunctionFlags = flags;
		}

		// Your own entry in the lobby's player list, renamed again after every rebuild of that list.
		void rename_lobby_entry(bool force)
		{
			static auto last = clock::time_point{};
			if (!force && clock::now() - last < 1s)
				return;
			last = clock::now();

			if (g_spoof_name.empty() || g_real_name.empty() || g_spoof_name == g_real_name)
				return;

			if (!players_list(false))
				return;

			const auto entry_class = SDK::UW_PlayersList_Entry_C::StaticClass();
			for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
			{
				auto object = SDK::UObject::GObjects->GetByIndex(i);
				if (!object || object->IsDefaultObject() || !object->IsA(entry_class))
					continue;

				auto entry = static_cast<SDK::UW_PlayersList_Entry_C*>(object);
				if (!entry->PlayerName)
					continue;

				const auto entry_name = entry->Player.DisplayName.ToString();
				const auto real_name = entry->Player.RealName.ToString();

				// Once the game rebuilds the list from the player state the entry already carries the new name, so
				// both names count as ours; the text is only replaced while the list still shows the old one.
				const bool ours = entry_name == g_real_name || real_name == g_real_name
				    || entry_name == g_spoof_name || real_name == g_spoof_name;
				if (!ours)
					continue;

				if (entry_name == g_spoof_name)
					continue;

				// Read what the entry shows instead of remembering the widget: the list is built again with a
				// new entry now and then, and there can be more than one entry for the same player.
				const auto name_text = static_cast<SDK::UTextBlock*>(entry->PlayerName);
				if (name_text->Text.TextData && name_text->Text.ToString() == g_spoof_name)
					continue;

				set_text(entry->PlayerName, g_spoof_name);
			}
		}

		// The lobby the tag was last rebuilt in, and with which name: a rejoin brings a new player state whose
		// name is the Steam account's until the game's own rename lands, so the tag is built before it is right.
		weak_object<SDK::APlayerState> g_name_tag_state;
		std::string g_name_tag_name;
		int g_name_tag_passes = 0;

		// A zero-argument game event, reached by name for the same reason as TextBlock.SetText: the SDK only
		// compiles some of the game's packages.
		bool run_event(SDK::UObject* object, const char* class_name, const char* function_name)
		{
			auto function = object->Class->GetFunction(class_name, function_name);
			if (!function)
				return false;

			const auto flags = function->FunctionFlags;
			function->FunctionFlags |= 0x400; // FUNC_Native, as the generated code sets before calling a game function
			object->ProcessEvent(function, nullptr);
			function->FunctionFlags = flags;
			return true;
		}

		// The name on a character's back belongs to the armour item it hangs on: the game spawns that extra, name
		// already drawn, when the item's skin is applied — once, so a rename or a rejoin leaves the old name there
		// until the loadout rebuilds the character. Re-running the item's own skin update builds the tag again.
		void refresh_name_tags()
		{
			if (g_spoof_name.empty())
				return;

			auto state = own_state();
			if (!state || state->PlayerNamePrivate.ToString() != g_spoof_name)
				return; // the game has not taken the name yet, so a tag built now would read the old one

			if (g_name_tag_state == state && g_name_tag_name == g_spoof_name)
				return;

			static auto last = clock::time_point{};
			if (clock::now() - last < 2s)
				return;
			last = clock::now();

			const auto tag_class = SDK::ANameTag::StaticClass();
			const auto character_class = SDK::AReadyOrNotCharacter::StaticClass();

			int tags = 0;
			for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
			{
				auto object = SDK::UObject::GObjects->GetByIndex(i);
				if (!object || object->IsDefaultObject() || !object->IsA(tag_class))
					continue;

				// Only the tag worn by our own character is ours; the ones on other players carry their names.
				auto tag = static_cast<SDK::ANameTag*>(object);
				++tags;
				auto parent = tag->GetAttachParentActor();
				if (!parent || !unreal_engine::is_valid_actor(parent) || !parent->IsA(character_class))
					continue;
				if (static_cast<SDK::AReadyOrNotCharacter*>(parent)->PlayerState != state)
					continue;

				auto item = tag->Owner;
				if (!item || !unreal_engine::is_valid_actor(item) || !item->IsA(SDK::ABaseItem::StaticClass()))
					continue;

				const bool applied = run_event(item, "BaseItem", "OnRep_Skin");
				LOG(INFO) << "Change Name: " << (applied ? "rebuilt" : "could not rebuild") << " the tag on the back of "
				          << item->GetName();

				g_name_tag_state = state;
				g_name_tag_name = g_spoof_name;
				g_name_tag_passes = 0;
				return;
			}

			// The character may still be loading; a new tag is looked at on a later pass.
			if (!tags && ++g_name_tag_passes >= 60)
			{
				g_name_tag_state = state;
				g_name_tag_name = g_spoof_name;
			}
		}

		void apply_name(const std::string& name)
		{
			auto state = own_state();
			if (!state)
			{
				notification::warning("Change Name", "There is no player of yours to rename yet.");
				return;
			}

			const auto current = state->PlayerNamePrivate.ToString();
			if (g_real_name.empty())
				g_real_name = current;

			g_spoof_name = name.empty() ? g_real_name : name;
			g_spoof_attempts = 0;
			g_spoof_state = state;

			// The platform follows the name.
			g_platform_type = g_spoof_name == g_real_name ? 4000 : 0;
			g_platform_type_set = true;
			g_platform_attempts = 0;

			if (g_spoof_name == current)
			{
				notification::info("Change Name", std::format("The name stays {}.", current));
				return;
			}

			ask_server_for_name(state);
			rename_lobby_entry(true);
			apply_platform_fields();
			notification::success("Change Name", std::format("Asked the game for {}; an empty box puts {} back.", g_spoof_name, g_real_name));
		}

		// The host is the server, so the name it writes on another player's state is the one that reaches every
		// client; the game then republishes it as usual.
		void apply_other_name(SDK::APlayerState* state, const std::string& name)
		{
			if (!ron::require_host("Change Name"))
				return;

			if (name.empty())
			{
				notification::warning("Change Name", "Type the name to give them first.");
				return;
			}

			if (!unreal_engine::is_valid_actor(state) || state == own_state())
				return;

			// A copy the game's own allocator owns, so the property can be replaced and freed as usual later.
			const auto wide = widen(name);
			const SDK::FString value(wide.c_str());
			state->PlayerNamePrivate = SDK::UKismetStringLibrary::Concat_StrStr(value, SDK::FString(L""));

			notification::success("Change Name", std::format("Their name is {} now, for every client.", name));
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

		SDK::UStatSubsystem* stat_subsystem()
		{
			auto world = SDK::UWorld::GetWorld();
			return world
			    ? static_cast<SDK::UStatSubsystem*>(
			          SDK::USubsystemBlueprintLibrary::GetGameInstanceSubsystem(world, SDK::UStatSubsystem::StaticClass()))
			    : nullptr;
		}

		// One career stat as the last read on the game thread saw it.
		std::mutex g_stats_mutex;
		std::vector<stat_info> g_stats;
		std::atomic<bool> g_stats_wanted = false;

		// The names a text stat can hold, per loadout slot, read once from the same lists the give item panel shows.
		std::mutex g_choices_mutex;
		std::map<std::string, std::vector<std::string>> g_choices;

		void read_stat_choices()
		{
			auto world = SDK::UWorld::GetWorld();
			auto manager = world ? SDK::ULoadoutManager::Get(world) : nullptr;
			if (!manager)
				return;

			std::map<std::string, std::vector<std::string>> choices;
			auto collect = [&choices](const char* slot, SDK::TArray<SDK::ABaseItem*>& items) {
				auto& names = choices[slot];
				for (auto item : items)
				{
					if (!item || !item->ItemName.TextData)
						continue;

					auto name = item->ItemName.ToString();
					if (name.empty() || std::find(names.begin(), names.end(), name) != names.end())
						continue;

					names.push_back(std::move(name));
				}
			};

			collect("primary", manager->PrimaryWeapons);
			collect("secondary", manager->SecondaryWeapons);
			collect("longtactical", manager->LongTacticalItems);
			collect("armor", manager->BodyArmors);

			// Difficulty names come from the game's own list; the stat holds them lower case, such as "casual", so the
			// leaf of each tag is lower cased here.
			SDK::TArray<SDK::FGameplayTag> difficulties;
			if (SDK::UBpGameplayHelperLib::GetAllDifficultyNames(&difficulties))
			{
				auto& names = choices["difficulty"];
				for (const auto& tag : difficulties)
				{
					// A gameplay tag is only its name, which the dumper leaves protected, so it is read as the name.
					const auto full = reinterpret_cast<const SDK::FName&>(tag).ToString();
					auto leaf = full.substr(full.find_last_of('.') + 1);
				std::transform(leaf.begin(), leaf.end(), leaf.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (!leaf.empty() && std::find(names.begin(), names.end(), leaf) == names.end())
						names.push_back(std::move(leaf));
				}
			}

			// Mission names are the level assets' own names, which is what the stat holds. The levels are asked for
			// first, since the game only keeps them resident from the mission select onwards.
			const auto levels = SDK::UBpGameplayHelperLib::GetLevels();
			auto& missions = choices["mission"];
			for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
			{
				auto object = SDK::UObject::GObjects->GetByIndex(i);
				if (!object || !object->IsA(SDK::ULevelData::StaticClass()))
					continue;

				auto name = static_cast<SDK::ULevelData*>(object)->LevelName.ToString();
				if (name.empty() || std::find(missions.begin(), missions.end(), name) != missions.end())
					continue;

				missions.push_back(std::move(name));
			}

			// No level asset was resident, so the names the game shows for them are offered instead.
			if (missions.empty())
			{
				for (const auto& level : levels)
				{
					auto name = level.FriendlyLevelName.TextData ? level.FriendlyLevelName.ToString() : std::string{};
					if (name.empty() || std::find(missions.begin(), missions.end(), name) != missions.end())
						continue;

					missions.push_back(std::move(name));
				}
			}

			std::sort(missions.begin(), missions.end());
			std::sort(choices["difficulty"].begin(), choices["difficulty"].end());

			std::lock_guard lock(g_choices_mutex);
			g_choices = std::move(choices);
		}

		void read_stats()
		{
			auto subsystem = stat_subsystem();
			auto table = subsystem && subsystem->StatsDataTableWrapper ? subsystem->StatsDataTableWrapper->StatDefinitions : nullptr;
			if (!subsystem || !table)
				return;

			// The loadout names behind the text stats are read once; those lists only change between missions.
			static std::atomic<bool> choices_read = false;
			if (!choices_read.exchange(true))
				read_stat_choices();

			std::vector<stat_info> stats;
			for (int i = 0; i < table->RowMap.Max(); ++i)
			{
				if (!table->RowMap.IsValidIndex(i))
					continue;

				const auto definition = reinterpret_cast<const SDK::FStatDefinition*>(table->RowMap[i].Value());
				if (!definition)
					continue;

				stat_info info;
				info.id = definition->StatID.ToString();
				if (info.id.empty())
					continue;

				info.label = definition->DisplayName.TextData ? definition->DisplayName.ToString() : std::string{};
				if (info.label.empty())
					info.label = info.id;
				info.type = static_cast<int>(definition->StatType);
				info.is_time = definition->IsTime;

				const auto wide = widen(info.id);
				const SDK::FString key(wide.c_str());
				info.numeric = subsystem->IsStatNumeric(key);
				info.value = subsystem->GetStatValue(key);
				info.text = subsystem->GetStatString(key).ToString();

				stats.push_back(std::move(info));
			}

			std::lock_guard lock(g_stats_mutex);
			g_stats = std::move(stats);
		}

		// An average holds the written number divided by the samples behind it, so it is set by writing the target times
		// that count. The count is measured with one large write, then corrected once if the read back is still off.
		void apply_average(SDK::UStatSubsystem* subsystem, const SDK::FString& key, const stat_info& stat, std::int64_t target)
		{
			constexpr std::int64_t probe_value = 10000000;

			subsystem->ClearStat(key);
			subsystem->UpdateStat(key, probe_value);
			const auto probe = subsystem->GetStatValue(key);
			const auto count = probe > 0 ? (probe_value + probe / 2) / probe : 1;

			subsystem->ClearStat(key);
			subsystem->UpdateStat(key, target * count);
			auto after = subsystem->GetStatValue(key);

			// A second pass, since the probe is rounded down and a stat with no samples at all reports zero.
			if (after > 0 && after != target)
			{
				const auto real = (target * count) / after;
				subsystem->ClearStat(key);
				subsystem->UpdateStat(key, target * (real > 0 ? real : 1));
				after = subsystem->GetStatValue(key);
			}

			notification::success("Career Stats", std::format("{} is now {}.", stat.label, after));
		}

		// An average is the written value divided by the samples behind it, so writing one would only scale it; the
		// other numerics take the value as it is once the stat is cleared, and favourite/text stats hold a string.
		void apply_stat(const stat_info& stat, const std::string& value)
		{
			auto subsystem = stat_subsystem();
			if (!subsystem)
			{
				notification::warning("Career Stats", "The stat subsystem is not up yet.");
				return;
			}

			const auto wide = widen(stat.id);
			const SDK::FString key(wide.c_str());

			if (stat.type == 5 || stat.type == 6 || !stat.numeric)
			{
				if (value.empty())
				{
					notification::warning("Career Stats", "Type the text to store first.");
					return;
				}

				const auto value_wide = widen(value);
				subsystem->UpdateStatString(key, SDK::FString(value_wide.c_str()));
				notification::success("Career Stats", std::format("{} is now {}.", stat.label, value));
			}
			else if (stat.type == 4)
			{
				apply_average(subsystem, key, stat, std::atoi(value.c_str()));
			}
			else
			{
				const auto number = std::atoi(value.c_str());
				subsystem->ClearStat(key);
				subsystem->UpdateStat(key, number);
				notification::success("Career Stats", std::format("{} is now {}.", stat.label, number));
			}

			g_stats_wanted = true;
		}

		void tick()
		{
			static auto last_read = clock::time_point{};
			if (clock::now() - last_read < 250ms)
				return;
			last_read = clock::now();

			// The stats page asks for a fresh copy every frame it is drawn, so the read happens only while it is open.
			if (g_stats_wanted.exchange(false))
				read_stats();

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
			keep_name();
			rename_lobby_entry(false);
			refresh_name_tags();
			keep_platform();

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

		void add_magazine(SDK::APlayerCharacter* target, const std::string& name)
		{
			if (const int rounds = ron::give_magazine(target))
				notification::success("Add Magazine", std::format("Gave {} a magazine of {} rounds.", name, rounds));
			else
				notification::warning("Add Magazine", std::format("{} is not holding a gun that takes magazines.", name));
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
			if (!ron::is_host() && (action == player_action::heal || action == player_action::add_magazine || action == player_action::godmode))
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
			case player_action::add_magazine:
				if (ron::require_host("Add Magazine"))
					add_magazine(target, name);
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

		// As the host: Heal & Revive, Add Magazine or Godmode a client asked for, on the player it named.
		host_link::client_action _player_action_for_clients("player_action", "Online Players", [](SDK::APlayerCharacter*, const std::string& args) {
			const auto colon = args.find(':');
			auto gs = ron::game_state();
			if (colon == std::string::npos || !gs)
				return false;

			const auto action = static_cast<player_action>(std::atoi(args.substr(0, colon).c_str()));
			if (action != player_action::heal && action != player_action::add_magazine && action != player_action::godmode)
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

	std::vector<stat_info> player_event::stats()
	{
		// The menu is drawing the page, so the game thread reads the list once on its next pass.
		g_stats_wanted = true;

		std::lock_guard lock(g_stats_mutex);
		return g_stats;
	}

	void player_event::set_stat(const stat_info& stat, const std::string& value)
	{
		async_run([stat, value] {
			apply_stat(stat, value);
		});
	}

	std::vector<std::string> player_event::stat_choices(const std::string& id)
	{
		// A favourite weapon is stored by the name the loadout screen gives it, so the names are offered instead of a
		// free text box; every other text stat, such as a mission or a difficulty, is typed.
		for (const auto* slot : {"primary", "secondary", "longtactical", "armor", "mission", "difficulty"})
		{
			if (id.find(slot) == std::string::npos)
				continue;

			std::lock_guard lock(g_choices_mutex);
			const auto found = g_choices.find(slot);
			return found == g_choices.end() ? std::vector<std::string>{} : found->second;
		}

		return {};
	}

	void player_event::rename(weak_object<SDK::APlayerState> state, const std::string& name)
	{
		auto trimmed = name;
		const auto first = trimmed.find_first_not_of(' ');
		const auto last = trimmed.find_last_not_of(' ');
		trimmed = first == std::string::npos ? std::string{} : trimmed.substr(first, last - first + 1);
		if (trimmed.size() > 32)
			trimmed.resize(32);

		async_run([state, trimmed] {
			auto target = state.get();
			if (!target)
			{
				notification::warning("Change Name", "That player has left.");
				return;
			}

			if (target == own_state())
				apply_name(trimmed);
			else
				apply_other_name(target, trimmed);
		});
	}
}
