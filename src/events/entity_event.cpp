#include "entity_event.hpp"
#include "script.hpp"
#include "features/features.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "SDK/Engine_parameters.hpp"

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

	// The skeleton's joints by bone name, which the AI's meshes share while each orders its bones differently.
	enum joint : int
	{
		head_end, head, neck, spine_3, spine_2, spine_1, pelvis,
		clavicle_l, upperarm_l, lowerarm_l, hand_l, clavicle_r, upperarm_r, lowerarm_r, hand_r,
		thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r, joint_count
	};

	static constexpr std::pair<joint, joint> skeleton_lines[] = {
	    {head, neck}, {neck, spine_3}, {spine_3, spine_2}, {spine_2, spine_1}, {spine_1, pelvis},
	    {spine_3, clavicle_l}, {clavicle_l, upperarm_l}, {upperarm_l, lowerarm_l}, {lowerarm_l, hand_l},
	    {spine_3, clavicle_r}, {clavicle_r, upperarm_r}, {upperarm_r, lowerarm_r}, {lowerarm_r, hand_r},
	    {pelvis, thigh_l}, {thigh_l, calf_l}, {calf_l, foot_l},
	    {pelvis, thigh_r}, {thigh_r, calf_r}, {calf_r, foot_r},
	};

	static const std::array<SDK::FName, joint_count>& joint_names()
	{
		static const auto names = [] {
			constexpr const wchar_t* text[joint_count] = {L"head_end", L"Head", L"neck_1", L"spine_3", L"spine_2", L"spine_1", L"pelvis",
			    L"clavicle_LE", L"upperarm_LE", L"lowerarm_LE", L"hand_LE", L"clavicle_RI", L"upperarm_RI", L"lowerarm_RI", L"hand_RI",
			    L"thigh_LE", L"calf_LE", L"foot_LE", L"thigh_RI", L"calf_RI", L"foot_RI"};
			std::array<SDK::FName, joint_count> out;
			for (int i = 0; i < joint_count; i++)
				out[i] = SDK::UKismetStringLibrary::Conv_StringToName(text[i]);
			return out;
		}();
		return names;
	}

	// Which joints the mesh has, a bit each, asked once per mesh asset. The asset's object index is kept too, since a
	// freed asset's address can be reused.
	static std::uint32_t joints_of(SDK::USkeletalMeshComponent* mesh)
	{
		auto asset = mesh->SkeletalMeshAsset;
		if (!asset)
			return 0;

		struct cached
		{
			std::int32_t index;
			std::uint32_t joints;
		};
		static std::unordered_map<SDK::UObject*, cached> meshes;
		auto it = meshes.find(asset);
		if (it == meshes.end() || it->second.index != asset->Index)
		{
			std::uint32_t joints = 0;
			for (int i = 0; i < joint_count; i++)
			{
				if (mesh->GetBoneIndex(joint_names()[i]) != -1)
					joints |= 1u << i;
			}
			it = meshes.insert_or_assign(asset, cached{asset->Index, joints}).first;
		}
		return it->second.joints;
	}

	static bool build_skeleton(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, esp_data& data)
	{
		auto mesh = actor->Mesh;
		const auto joints = mesh ? joints_of(mesh) : 0;
		if (!joints)
			return false;

		// Each joint is placed once and shared by its lines.
		std::array<std::optional<SDK::FVector2D>, joint_count> screen;
		for (int i = 0; i < joint_count; i++)
		{
			if (!(joints & (1u << i)))
				continue;

			SDK::FVector2D at;
			const auto world = mesh->GetBoneTransform(joint_names()[i], SDK::ERelativeTransformSpace::RTS_World).Translation;
			if (controller->ProjectWorldLocationToScreen(world, &at, true))
				screen[i] = at;
		}

		for (auto [a, b] : skeleton_lines)
		{
			if (screen[a] && screen[b])
				data.skeleton.push_back({*screen[a], *screen[b]});
		}

		// The head is a circle from the Head bone, at the base of the skull, to the top of it.
		if (screen[head] && screen[head_end])
		{
			const auto& base = *screen[head];
			const auto& top = *screen[head_end];
			data.head = {(base.X + top.X) / 2, (base.Y + top.Y) / 2};
			data.head_radius = (std::max)(static_cast<float>(std::hypot(top.X - base.X, top.Y - base.Y)) / 2.f, 2.f);
		}
		return !data.skeleton.empty();
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

	// The engine fills the path into an array it allocates itself, so one set of parameters is kept and refilled in place
	// rather than a new array left behind every frame.
	static SDK::Params::GameplayStatics_Blueprint_PredictProjectilePath_Advanced g_predict{};

	// Where the grenade in your hand would fly if thrown now, up to what it first hits, as the engine's physics predicts
	// it. Logged from thrown flashbangs, stingers, CS gas and 9-bangers: it leaves about 64 cm in front of the camera
	// along your aim, which the camera's sway can be a degree off, at ThrowImpulse plus UpImpulse straight up, both over
	// the 0.16 kg every grenade's body has in flight; in the hand the body reads 0.17 to 0.63 kg, which the game does not
	// throw with. The engine then lets it fall at the world's gravity and sweeps it as a ball of its size, 4 s at most,
	// against what blocks the grenade's own collision channel; it leaves out the body's 0.01 linear damping, which moves
	// the first hit up to 10 cm on the longest throws. Its bounces are left out: it spins as it flies, so the first floor
	// bounce kept 4 to 46% of the falling speed and 26 to 63% of the speed along the floor, different every throw.
	static void build_grenade_path(SDK::APlayerController* controller, std::vector<bone_line>& arc)
	{
		auto character = unreal_engine::get_character();
		auto item = character ? character->GetEquippedItem() : nullptr;
		if (!item || !item->IsA(SDK::ABaseGrenade::StaticClass()) || !controller->PlayerCameraManager)
			return;

		const auto grenade = static_cast<SDK::ABaseGrenade*>(item);
		const auto mesh = grenade->ItemMesh;
		constexpr double mass = 0.16;
		const auto channel = mesh ? mesh->GetCollisionObjectType() : SDK::ECollisionChannel::ECC_PhysicsBody;

		const auto aim = SDK::UKismetMathLibrary::GetForwardVector(controller->ControlRotation);
		static SDK::AActor* ignored[2];
		ignored[0] = character;
		ignored[1] = grenade;

		auto& params = g_predict.PredictParams;
		params.StartLocation = controller->PlayerCameraManager->CameraCachePrivate.POV.Location + aim * 64.0;
		params.LaunchVelocity = aim * (grenade->ThrowImpulse / mass);
		params.LaunchVelocity.Z += grenade->UpImpulse / mass;
		params.bTraceWithCollision = true;
		params.ProjectileRadius = 4.f;
		params.MaxSimTime = 4.f;
		params.bTraceWithChannel = true;
		params.TraceChannel = channel;
		params.ActorsToIgnore = SDK::TArray<SDK::AActor*>(ignored, 2, 2);
		params.SimFrequency = 30.f;
		params.OverrideGravityZ = 0.f;
		params.DrawDebugType = SDK::EDrawDebugTrace::None;
		params.bTraceComplex = false;
		g_predict.WorldContextObject = character;

		static auto function = SDK::UGameplayStatics::StaticClass()->GetFunction("GameplayStatics", "Blueprint_PredictProjectilePath_Advanced");
		const auto flags = function->FunctionFlags;
		function->FunctionFlags |= 0x400;
		SDK::UGameplayStatics::GetDefaultObj()->ProcessEvent(function, &g_predict);
		function->FunctionFlags = flags;

		SDK::FVector2D from_screen;
		bool from_shown = false;
		for (const auto& point : g_predict.PredictResult.PathData)
		{
			SDK::FVector2D to_screen;
			const bool to_shown = controller->ProjectWorldLocationToScreen(point.Location, &to_screen, false);
			if (from_shown && to_shown)
				arc.push_back({ from_screen, to_screen });

			from_screen = to_screen;
			from_shown = to_shown;
		}

		SDK::FVector2D hit_screen;
		if (g_predict.ReturnValue && controller->ProjectWorldLocationToScreen(g_predict.PredictResult.HitResult.Location, &hit_screen, false))
			arc.push_back({ hit_screen, hit_screen });
	}

	static void build_frame()
	{
		auto& back = g_esp_data.back();
		auto& radar = g_radar_data.back();
		auto& arc = g_grenade_path.back();
		back.clear();
		radar.clear();
		arc.clear();

		auto gs = ron::game_state();
		auto controller = unreal_engine::get_player_controller();
		if (!ron::is_in_mission() || !gs || !controller)
		{
			g_static_targets.clear();
			g_esp_data.publish();
			g_radar_data.publish();
			g_grenade_path.publish();
			return;
		}

		if (_grenade_path.get_state())
			build_grenade_path(controller, arc);

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

		// Recorded before the ESP's own distance and screen checks, since the radar also shows what is behind you.
		const bool radar_on = _radar.get_state();
		const float radar_range = _radar_range.get_state();
		const double yaw = controller->ControlRotation.Yaw * (M_PI / 180.0);
		const double cos_yaw = std::cos(yaw);
		const double sin_yaw = std::sin(yaw);
		auto add_radar = [&](const SDK::FVector& location, float distance, Color color) {
			if (!radar_on || distance > radar_range)
				return;

			const auto delta = location - origin;
			radar.push_back({ static_cast<float>((delta.X * cos_yaw + delta.Y * sin_yaw) / 100.0),
			    static_cast<float>((delta.Y * cos_yaw - delta.X * sin_yaw) / 100.0), std::abs(delta.Z) > 250.0, color });
		};

		for (const auto& target : g_static_targets)
		{
			if (!kind_enabled(target.kind))
				continue;

			const float distance = origin.GetDistanceToInMeters(target.location);
			add_radar(target.location, distance, target.color);
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
			const auto status = character->CharacterHealth ? character->CharacterHealth->HealthStatus : SDK::EPlayerHealthStatus::HS_NotAvailable;
			const auto color = character_color(kind, status);
			add_radar(location, distance, color);
			if (distance > max_distance)
				return;

			// Labels sit at the feet, under the box.
			if (auto feet = unreal_engine::get_location_bone(character, EBonesIndex::Root); feet.X != 0.f || feet.Y != 0.f || feet.Z != 0.f)
				location = feet;

			SDK::FVector2D screen;
			if (!controller->ProjectWorldLocationToScreen(location, &screen, false))
				return;

			esp_data data{};
			data.kind = kind;
			data.screen = screen;
			data.distance = distance;
			data.color = color;
			data.display_text = std::format("{} [{}] [{:.0f}m]", name, ron::health_status_name(status), distance);

			if (_draw_health.get_state())
			{
				data.health = character->GetCurrentHealth();
				data.max_health = character->GetMaxHealth();
			}

			// The bone pairs are for the AI's meshes only.
			if (_draw_skeleton.get_state() && kind != esp_kind::player)
				data.has_skeleton = build_skeleton(character, controller, data);
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
		g_radar_data.publish();
		g_grenade_path.publish();
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
				g_radar_data.clear_all();
				g_grenade_path.clear_all();
				g_static_targets.clear();
				had_data = false;
			}

			co_await this_task::wait();
		}

		g_esp_data.clear_all();
		g_radar_data.clear_all();
		g_grenade_path.clear_all();
		co_return;
	}
}
