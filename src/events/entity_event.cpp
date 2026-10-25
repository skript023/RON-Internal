#include "entity_event.hpp"
#include "script.hpp"
#include "features/features.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"

namespace big
{
	using namespace features;

	struct name_map_entry
	{
		std::string_view key;
		std::string_view value;
	};

	// Weapons
	constexpr std::array<name_map_entry, 17> weapons{ {
		{ "Primary_M249_C", "M249" },
		{ "Primary_M76_C", "M76" },
		{ "Primary_CAR_C", "CAR" },
		{ "Primary_SBR_C", "SBR" },
		{ "Primary_MP5A3_C", "MP5A3" },
		{ "Primary_MP7_C", "MP7" },
		{ "Primary_BRN180_C", "BRN180" },
		{ "Primary_SCARL_v2_C", "SCARL" },
		{ "Primary_Benelli_M4_C", "M4" },
		{ "Primary_UMP45_v2_C", "UMP45" },
		{ "Primary_MP9_C", "MP9" },
		{ "Secondary_Makarov_2Handed_C", "Makarov" },
		{ "Secondary_Tec9_C", "Tec9" },
		{ "Secondary_Python_V2_C", "Python" },
		{ "Secondary_S_Python_C", "Python" },
		{ "Secondary_G19_V2_C", "G19" },
		{ "Secondary_Taser_V2_C", "Taser" }
	} };

	// Something that stays where it is, such as a trap or a piece of evidence. These are found by a scan twice a
	// second; only their projection runs every frame.
	struct static_target
	{
		esp_kind kind;
		SDK::FVector location;
		std::string label;
		Color color;
	};

	static std::vector<static_target> g_static_targets;

	static const char* item_label(SDK::ABaseItem* item)
	{
		static std::unordered_map<SDK::UClass*, const char*> labels;
		if (auto it = labels.find(item->Class); it != labels.end())
			return it->second;

		const char* label = "Evidence";
		if (item->IsA(SDK::ABaseWeapon::StaticClass()))
		{
			label = "Weapon";
			const auto name = item->Class->GetName();
			for (const auto& e : weapons)
			{
				if (name.contains(e.key))
				{
					label = e.value.data();
					break;
				}
			}
		}

		labels.emplace(item->Class, label);
		return label;
	}

	enum class level_kind : std::uint8_t
	{
		none,
		trap,
		collectible
	};

	// Classes never change, so each one is looked up once instead of walking its class chain for every actor.
	static level_kind classify_level_actor(SDK::UClass* cls)
	{
		static std::unordered_map<SDK::UClass*, level_kind> cache;
		if (auto it = cache.find(cls); it != cache.end())
			return it->second;

		auto kind = level_kind::none;
		if (cls->IsSubclassOf(SDK::ATrapActor::StaticClass()))
			kind = level_kind::trap;
		else if (cls->IsSubclassOf(SDK::ACollectable::StaticClass()))
			kind = level_kind::collectible;

		cache.emplace(cls, kind);
		return kind;
	}

	static std::optional<esp_kind> classify_character(SDK::UClass* cls)
	{
		static std::unordered_map<SDK::UClass*, std::optional<esp_kind>> cache;
		if (auto it = cache.find(cls); it != cache.end())
			return it->second;

		std::optional<esp_kind> kind;
		if (cls->IsSubclassOf(SDK::ASuspectCharacter::StaticClass()))
			kind = esp_kind::suspect;
		else if (cls->IsSubclassOf(SDK::ACivilianCharacter::StaticClass()))
			kind = esp_kind::civilian;
		else if (cls->IsSubclassOf(SDK::ASWATCharacter::StaticClass()))
			kind = esp_kind::swat;

		cache.emplace(cls, kind);
		return kind;
	}

	template <typename F>
	static void for_each_persistent_character(F&& callback)
	{
		auto world = SDK::UWorld::GetWorld();
		if (!world || !world->PersistentLevel)
			return;

		auto& actors = world->PersistentLevel->Actors;
		for (int i = 0; i < actors.Num(); ++i)
		{
			if (!actors.IsValidIndex(i))
				continue;

			auto actor = actors[i];
			if (!unreal_engine::is_valid_actor(actor) || !actor->Class || !actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
				continue;

			auto kind = classify_character(actor->Class);
			if (!kind)
				continue;

			callback(static_cast<SDK::ACyberneticCharacter*>(actor), *kind);
		}
	}

	static bool kind_enabled(esp_kind kind)
	{
		switch (kind)
		{
		case esp_kind::suspect: return _esp_suspects.get_state();
		case esp_kind::civilian: return _esp_civilians.get_state();
		case esp_kind::swat: return _esp_swat.get_state();
		case esp_kind::trap: return _esp_traps.get_state();
		case esp_kind::door: return _esp_doors.get_state();
		case esp_kind::evidence_weapon:
		case esp_kind::evidence: return _esp_evidence.get_state();
		case esp_kind::reportable: return _esp_reportables.get_state();
		case esp_kind::collectible: return _esp_collectibles.get_state();
		}

		return false;
	}

	static void refresh_static_targets(SDK::AReadyOrNotGameState* gs)
	{
		g_static_targets.clear();

		auto add = [](esp_kind kind, SDK::AActor* actor, std::string label, Color color) {
			g_static_targets.push_back({ kind, actor->K2_GetActorLocation(), std::move(label), color });
		};

		if (_esp_evidence.get_state())
		{
			// Dropped suspect weapons and items are evidence until someone picks them up.
			for (auto item : gs->AllItems)
			{
				if (!unreal_engine::is_valid_actor(item) || !item->EvidenceComponent || !item->EvidenceComponent->CanBeCollected())
					continue;

				add(esp_kind::evidence_weapon, item, item_label(item), { 255, 255, 0, 255 });
			}

			for (auto evidence : gs->AllEvidenceActors)
			{
				if (!unreal_engine::is_valid_actor(evidence))
					continue;
				if (evidence->EvidenceComponent && !evidence->EvidenceComponent->CanBeCollected())
					continue;

				auto name = ron::text(evidence->EvidenceName);
				add(esp_kind::evidence, evidence, name.empty() ? "Evidence" : name, { 255, 220, 0, 255 });
			}
		}

		if (_esp_reportables.get_state())
		{
			for (auto reportable : gs->AllReportableActors)
			{
				if (!unreal_engine::is_valid_actor(reportable) || reportable->bHasBeenReported || !reportable->bReportableEnabled)
					continue;

				auto name = ron::text(reportable->ReportableName);
				add(esp_kind::reportable, reportable, name.empty() ? "Reportable" : name, { 0, 200, 255, 255 });
			}
		}

		// A door's trap is drawn with the other traps; this shows what else stops it opening.
		if (_esp_doors.get_state())
		{
			for (auto door : gs->AllDoors)
			{
				if (!unreal_engine::is_valid_actor(door))
					continue;

				const bool locked = door->IsLocked();
				const bool wedged = door->GetAttachedWedge() != nullptr;
				if (locked || wedged)
					add(esp_kind::door, door, locked && wedged ? "Locked, Wedged" : locked ? "Locked" : "Wedged", { 235, 235, 235, 255 });
			}
		}

		if (!_esp_traps.get_state() && !_esp_collectibles.get_state())
			return;

		auto world = SDK::UWorld::GetWorld();
		if (!world)
			return;

		// Every loaded level, not only the persistent one: maps stream parts of the building in.
		for (auto level : world->Levels)
		{
			if (!level)
				continue;

			for (auto actor : level->Actors)
			{
				if (!unreal_engine::is_valid_actor(actor) || !actor->Class)
					continue;

				switch (classify_level_actor(actor->Class))
				{
				case level_kind::trap:
				{
					auto trap = static_cast<SDK::ATrapActor*>(actor);
					if (_esp_traps.get_state() && trap->TrapStatus == SDK::ETrapState::TS_Live)
						add(esp_kind::trap, trap, ron::trap_type_name(trap->TrapType), { 255, 120, 0, 255 });
					break;
				}
				case level_kind::collectible:
				{
					if (!_esp_collectibles.get_state())
						break;

					auto collectible = static_cast<SDK::ACollectable*>(actor);
					auto name = ron::text(collectible->ItemName);
					add(esp_kind::collectible, collectible, name.empty() ? "Collectible" : name, { 200, 120, 255, 255 });
					break;
				}
				default:
					break;
				}
			}
		}
	}

	static bool is_skeleton_variant_2(SDK::USkeletalMeshComponent* mesh)
	{
		if (!mesh) return false;

		auto name = mesh->GetBoneName(45).ToString();
		return name == "Head";
	}

	static bool build_skeleton(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, std::vector<bone_line>& out)
	{
		if (!actor || !controller) return false;

		auto mesh = actor->Mesh;
		if (!mesh) return false;

		bool variant2 = is_skeleton_variant_2(mesh);

		const auto& skeleton =
			actor->DefaultTeam == SDK::ETeamType::TT_SUSPECT
			? (variant2 ? suspect_skel_2 : suspect_skel_1)
			: (variant2 ? civ_skel_2 : civ_skel_1);

		for (auto [a, b] : skeleton)
		{
			SDK::FVector wa, wb;
			if (!unreal_engine::get_bone_world(actor, a, wa)) continue;
			if (!unreal_engine::get_bone_world(actor, b, wb)) continue;

			SDK::FVector2D sa, sb;

			if (!controller->ProjectWorldLocationToScreen(wa, &sa, true)) continue;
			if (!controller->ProjectWorldLocationToScreen(wb, &sb, true)) continue;

			out.push_back({ sa, sb });
		}

		return !out.empty();
	}

	static bool build_box(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, float& x, float& y, float& w, float& h)
	{
		SDK::FVector top = unreal_engine::get_location_bone(actor, L"Head");
		SDK::FVector bottom = unreal_engine::get_location_bone(actor, L"Root");

		top.Z += 15.f;

		SDK::FVector2D t2d, b2d;

		if (!controller->ProjectWorldLocationToScreen(top, &t2d, true)) return false;
		if (!controller->ProjectWorldLocationToScreen(bottom, &b2d, true)) return false;

		h = fabs(b2d.Y - t2d.Y);
		w = h * 0.45f;

		x = b2d.X - w / 2.f;
		y = t2d.Y;

		return true;
	}

	static bool build_box_3d(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, std::array<SDK::FVector2D, 8>& out)
	{
		if (!actor || !controller)
			return false;

		SDK::FVector origin, extent;
		actor->GetActorBounds(true, &origin, &extent, false);

		// 8 corners world
		SDK::FVector corners[8] = {
			origin + SDK::FVector(-extent.X, -extent.Y, -extent.Z),
			origin + SDK::FVector(-extent.X,  extent.Y, -extent.Z),
			origin + SDK::FVector(extent.X,  extent.Y, -extent.Z),
			origin + SDK::FVector(extent.X, -extent.Y, -extent.Z),

			origin + SDK::FVector(-extent.X, -extent.Y,  extent.Z),
			origin + SDK::FVector(-extent.X,  extent.Y,  extent.Z),
			origin + SDK::FVector(extent.X,  extent.Y,  extent.Z),
			origin + SDK::FVector(extent.X, -extent.Y,  extent.Z),
		};

		// W2S
		for (int i = 0; i < 8; i++)
		{
			if (!controller->ProjectWorldLocationToScreen(corners[i], &out[i], true))
				return false;
		}

		return true;
	}

	static Color character_color(esp_kind kind, SDK::EPlayerHealthStatus status)
	{
		using enum SDK::EPlayerHealthStatus;
		if (status == HS_Dead || status == HS_Incapacitated || status == HS_Arrested)
			return { 150, 150, 150, 200 };

		switch (kind)
		{
		case esp_kind::suspect: return { 255, 0, 0, 255 };
		case esp_kind::civilian: return { 0, 255, 0, 255 };
		case esp_kind::player: return { 0, 230, 230, 255 };
		default: return { 80, 160, 255, 255 };
		}
	}

	static void build_frame()
	{
		auto& back = g_esp_data.back();
		back.clear();

		auto gs = ron::game_state();
		auto controller = unreal_engine::get_player_controller();
		if (!ron::is_in_mission() || !gs || !controller)
		{
			g_static_targets.clear();
			g_esp_data.publish();
			return;
		}

		static auto last_scan = std::chrono::steady_clock::time_point{};
		static const void* last_game_state = nullptr;
		const auto now = std::chrono::steady_clock::now();
		if (gs != last_game_state || now - last_scan > 500ms)
		{
			refresh_static_targets(gs);
			last_scan = now;
			last_game_state = gs;
		}

		const auto origin = player::get_player_coords();
		const float max_distance = _esp_distance.get_state();

		for (const auto& target : g_static_targets)
		{
			if (!kind_enabled(target.kind))
				continue;

			const float distance = origin.GetDistanceToInMeters(target.location);
			if (distance > max_distance)
				continue;

			SDK::FVector2D screen;
			if (!controller->ProjectWorldLocationToScreen(target.location, &screen, false))
				continue;

			esp_data data{};
			data.kind = target.kind;
			data.screen = screen;
			data.distance = distance;
			data.display_text = std::format("{} [{:.0f}m]", target.label, distance);
			data.color = target.color;
			back.push_back(std::move(data));
		}

		const bool want_box = _draw_box.get_state() || _draw_corner_box.get_state() || _draw_health.get_state();

		auto add_character = [&](SDK::AReadyOrNotCharacter* character, esp_kind kind, std::string_view name) {
			auto location = character->K2_GetActorLocation();
			const float distance = origin.GetDistanceToInMeters(location);
			if (distance > max_distance)
				return;

			// Labels sit at the feet, under the box.
			if (auto feet = unreal_engine::get_location_bone(character, EBonesIndex::Root); feet.X != 0.f || feet.Y != 0.f || feet.Z != 0.f)
				location = feet;

			SDK::FVector2D screen;
			if (!controller->ProjectWorldLocationToScreen(location, &screen, false))
				return;

			const auto status = character->CharacterHealth ? character->CharacterHealth->HealthStatus : SDK::EPlayerHealthStatus::HS_NotAvailable;

			esp_data data{};
			data.kind = kind;
			data.screen = screen;
			data.distance = distance;
			data.color = character_color(kind, status);
			data.display_text = std::format("{} [{}] [{:.0f}m]", name, ron::health_status_name(status), distance);

			if (_draw_health.get_state())
			{
				data.health = character->GetCurrentHealth();
				data.max_health = character->GetMaxHealth();
			}

			// The bone pairs are for the AI's meshes only.
			if (_draw_skeleton.get_state() && kind != esp_kind::player)
				data.has_skeleton = build_skeleton(character, controller, data.skeleton);
			if (want_box)
				data.has_box = build_box(character, controller, data.box_x, data.box_y, data.box_w, data.box_h);
			if (_draw_box_3d.get_state())
				data.has_box_3d = build_box_3d(character, controller, data.box_3d);

			back.push_back(std::move(data));
		};

		for_each_persistent_character([&](SDK::ACyberneticCharacter* character, esp_kind kind) {
			if (character->Class && kind_enabled(kind))
				add_character(character, kind, ron::team_name(character));
		});

		if (_esp_players.get_state())
		{
			auto self = unreal_engine::get_character();
			for (auto character : gs->AllPlayerCharacters)
			{
				if (unreal_engine::is_valid_actor(character) && character != self && character->PlayerState)
					add_character(character, esp_kind::player, character->PlayerState->PlayerNamePrivate.ToString());
			}
		}

		g_esp_data.publish();
	}

	// A fault reading one actor skips this frame instead of ending the worker for the rest of the session.
	static void build_frame_guarded()
	{
		TRY_CLAUSE
		{
			build_frame();
		}
		EXCEPT_CLAUSE
	}

	task<void> entity_event::run()
	{
		LOG(INFO) << "Entity Event Registered";

		bool had_data = false;
		while (g_running)
		{
			if (_esp_enabled.get_state())
			{
				build_frame_guarded();
				had_data = true;
			}
			else if (had_data)
			{
				g_esp_data.clear_all();
				g_static_targets.clear();
				had_data = false;
			}

			co_await this_task::wait();
		}

		g_esp_data.clear_all();
		co_return;
	}
}
