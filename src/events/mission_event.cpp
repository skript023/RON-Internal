#include "mission_event.hpp"
#include "script.hpp"
#include "task_mgr.hpp"
#include "features/mission.hpp"
#include "features/host_link.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"

#include <astra/host/notification.hpp>

namespace big
{
	namespace
	{
		enum class target_action
		{
			arrest,
			surrender,
			knockout,
			unlock,
			lock,
			disarm_trap,
			remove_wedge,
			break_door,
			blow_up_door,
			disarm
		};

		// What the render thread may read. Everything else here belongs to the game thread.
		struct shared_state
		{
			std::mutex mutex;
			std::vector<astra::overlay_row> overlay;
			std::optional<astra::inspector> inspector;
			std::vector<target_action> actions;
			weak_object<SDK::ACyberneticCharacter> character;
			weak_object<SDK::ADoor> door;
			std::vector<collectible_entry> collectibles;
		};

		shared_state g_shared;

		struct tracked_objective
		{
			weak_object<SDK::AObjective> objective;
			SDK::EObjectiveStatus status;
		};

		std::vector<tracked_objective> g_objectives;
		const void* g_objectives_game_state = nullptr;

		std::string format_time(float seconds)
		{
			const int total = std::max(0, static_cast<int>(seconds));
			return std::format("{:02}:{:02}", total / 60, total % 60);
		}

		const char* objective_status_name(SDK::EObjectiveStatus status)
		{
			switch (status)
			{
			case SDK::EObjectiveStatus::Objective_Complete: return "Done";
			case SDK::EObjectiveStatus::Objective_Failed: return "Failed";
			default: return "In progress";
			}
		}

		bool can_collect(SDK::UEvidenceComponent* evidence)
		{
			return evidence && evidence->CanBeCollected();
		}

		// Tells the player when an objective changes state, comparing with what the last read saw.
		void check_objectives(SDK::AReadyOrNotGameState* gs)
		{
			if (g_objectives_game_state != gs)
			{
				g_objectives.clear();
				g_objectives_game_state = gs;
			}

			for (auto objective : gs->MissionObjectives)
			{
				if (!unreal_engine::is_valid_actor(objective))
					continue;

				const auto status = objective->GetObjectiveStatus();
				auto it = std::ranges::find_if(g_objectives, [objective](const tracked_objective& t) { return t.objective.get() == objective; });
				if (it == g_objectives.end())
				{
					g_objectives.push_back({ objective, status });
					continue;
				}

				if (it->status == status)
					continue;

				it->status = status;
				if (!_objective_alerts.get_state() || objective->bHiddenObjective)
					continue;

				const auto name = ron::text(objective->ObjectiveName);
				if (status == SDK::EObjectiveStatus::Objective_Complete)
					notification::success("Objective complete", name);
				else if (status == SDK::EObjectiveStatus::Objective_Failed)
					notification::error("Objective failed", name);
			}
		}

		std::vector<astra::overlay_row> build_overlay(SDK::AReadyOrNotGameState* gs)
		{
			int suspects = 0, suspects_active = 0, civilians = 0, civilians_secured = 0, arrested = 0, killed = 0, to_report = 0;
			ron::for_each_ai([&](SDK::ACyberneticCharacter* c) {
				const bool is_arrested = c->IsArrested();
				const bool is_down = c->IsDeadOrUnconscious() || c->IsIncapacitated();

				if (c->IsA(SDK::ASuspectCharacter::StaticClass()))
				{
					++suspects;
					suspects_active += !is_arrested && !is_down;
				}
				else if (c->IsA(SDK::ACivilianCharacter::StaticClass()))
				{
					++civilians;
					civilians_secured += is_arrested;
				}
				else
				{
					return;
				}

				arrested += is_arrested;
				killed += c->IsDeadNotUnconscious();
				to_report += !c->bHasBeenReported && (is_arrested || is_down);
			});

			int evidence = 0;
			for (auto item : gs->AllItems)
				evidence += unreal_engine::is_valid_actor(item) && can_collect(item->EvidenceComponent);
			for (auto actor : gs->AllEvidenceActors)
				evidence += unreal_engine::is_valid_actor(actor) && (!actor->EvidenceComponent || can_collect(actor->EvidenceComponent));

			for (auto reportable : gs->AllReportableActors)
				to_report += unreal_engine::is_valid_actor(reportable) && reportable->bReportableEnabled && !reportable->bHasBeenReported;

			int traps = 0;
			for (auto trap : gs->AllDoorTrapActors)
				traps += unreal_engine::is_valid_actor(trap) && trap->TrapStatus == SDK::ETrapState::TS_Live;

			std::vector<astra::overlay_row> objective_rows;
			int objectives = 0, objectives_done = 0;
			for (auto objective : gs->MissionObjectives)
			{
				if (!unreal_engine::is_valid_actor(objective) || objective->bHiddenObjective)
					continue;

				const auto status = objective->GetObjectiveStatus();
				++objectives;
				objectives_done += status == SDK::EObjectiveStatus::Objective_Complete;
				objective_rows.push_back({ ron::text(objective->ObjectiveName), objective_status_name(status) });
			}

			std::vector<astra::overlay_row> rows;
			if (auto mission = gs->MissionName.ToString(); !mission.empty())
				rows.push_back({ "Mission", mission });
			rows.push_back({ "Time", format_time(gs->RoundTimeElapsed) });
			rows.push_back({ "Suspects", std::format("{} of {} still active", suspects_active, suspects) });
			rows.push_back({ "Civilians", std::format("{} of {} secured", civilians_secured, civilians) });
			rows.push_back({ "Arrested", std::to_string(arrested) });
			rows.push_back({ "Killed", std::to_string(killed) });
			rows.push_back({ "To report", std::to_string(to_report) });
			rows.push_back({ "Evidence left", std::to_string(evidence) });
			rows.push_back({ "Live door traps", std::to_string(traps) });

			const int score = gs->GetCurrentSwatScore();
			const int max_score = gs->GetMaxSwatScore();
			rows.push_back({ "Score", max_score > 0 ? std::format("{} / {}", score, max_score) : std::to_string(score) });

			if (objectives > 0)
			{
				rows.push_back({ "Objectives", std::format("{} of {} done", objectives_done, objectives) });
				rows.insert(rows.end(), objective_rows.begin(), objective_rows.end());
			}

			return rows;
		}

		SDK::ACyberneticCharacter* character_under_crosshair(SDK::APlayerController* controller)
		{
			int width = 0, height = 0;
			controller->GetViewportSize(&width, &height);
			if (width <= 0 || height <= 0)
				return nullptr;

			const float center_x = width * 0.5f, center_y = height * 0.5f;
			const auto origin = player::get_player_coords();

			SDK::ACyberneticCharacter* best = nullptr;
			float best_distance = 120.f * 120.f; // pixels from the crosshair, squared
			ron::for_each_ai([&](SDK::ACyberneticCharacter* c) {
				const auto location = c->K2_GetActorLocation();
				if (origin.GetDistanceToInMeters(location) > 40.f)
					return;

				SDK::FVector2D screen;
				if (!controller->ProjectWorldLocationToScreen(location, &screen, false))
					return;

				const float dx = screen.X - center_x, dy = screen.Y - center_y;
				if (const float d = dx * dx + dy * dy; d < best_distance)
				{
					best = c;
					best_distance = d;
				}
			});

			return best;
		}

		// How far along the line from `from` in the unit direction `dir` it enters the box, if it does.
		std::optional<double> enters_box(const SDK::FVector& from, const SDK::FVector& dir, const SDK::FVector& centre, const SDK::FVector& extent)
		{
			const double o[] = {from.X, from.Y, from.Z};
			const double d[] = {dir.X, dir.Y, dir.Z};
			const double c[] = {centre.X, centre.Y, centre.Z};
			const double e[] = {extent.X, extent.Y, extent.Z};
			double enter = 0.0, leave = DBL_MAX;
			for (int i = 0; i < 3; ++i)
			{
				if (std::abs(d[i]) < 1e-9)
				{
					if (std::abs(o[i] - c[i]) > e[i])
						return std::nullopt;
					continue;
				}
				double in = (c[i] - e[i] - o[i]) / d[i], out = (c[i] + e[i] - o[i]) / d[i];
				if (in > out)
					std::swap(in, out);
				enter = (std::max)(enter, in);
				leave = (std::min)(leave, out);
				if (enter > leave)
					return std::nullopt;
			}
			return enter;
		}

		// The game sets bLookingAtDoor but leaves lastHighlightedDoor empty, so the door is the one your line of sight hits
		// within 15 m, or else the first door leaf it crosses up to a metre past what blocks the view: glass doors let a
		// trace through, and some doors sit inside the collision of the wall around them. The whole door also spans its
		// frame's interaction boxes and stays after the leaf is blown away, so only an unbroken leaf counts.
		SDK::ADoor* door_under_crosshair(SDK::APlayerController* controller, SDK::APlayerCharacter* player)
		{
			auto camera = controller->PlayerCameraManager;
			auto gs = ron::game_state();
			if (!camera || !gs)
				return nullptr;

			const auto& pov = camera->CameraCachePrivate.POV;
			const auto dir = SDK::UKismetMathLibrary::GetForwardVector(pov.Rotation);
			double reach = 1500.0;
			SDK::FHitResult hit{};
			if (SDK::UKismetSystemLibrary::LineTraceSingle(player, pov.Location, pov.Location + dir * reach, SDK::ETraceTypeQuery::TraceTypeQuery1, false,
			        {}, SDK::EDrawDebugTrace::None, &hit, true, {}, {}, 0.f))
			{
				// Someone in front of the door is who you look at.
				auto blocker = hit.Component.Get();
				auto owner = blocker ? blocker->GetOwner() : nullptr;
				if (owner && owner->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
					return nullptr;
				// Some doors are built of chunks alone and leave DoorStatic empty, so a door the line hits is taken as it is.
				if (owner && owner->IsA(SDK::ADoor::StaticClass()) && !static_cast<SDK::ADoor*>(owner)->IsDoorBroken())
					return static_cast<SDK::ADoor*>(owner);
				reach = (std::min)(reach, hit.Distance + 100.0);
			}

			SDK::ADoor* best = nullptr;
			double best_at = reach;
			for (auto door : gs->AllDoors)
			{
				if (!unreal_engine::is_valid_actor(door) || !door->DoorStatic)
					continue;

				SDK::FVector centre, extent;
				float radius;
				SDK::UKismetSystemLibrary::GetComponentBounds(door->DoorStatic, &centre, &extent, &radius);
				if (const auto at = enters_box(pov.Location, dir, centre, extent); at && *at < best_at && door->DoorStatic->IsVisible() && !door->IsDoorBroken())
				{
					best = door;
					best_at = *at;
				}
			}
			return best;
		}

		astra::inspector character_card(SDK::ACyberneticCharacter* c, std::vector<target_action>& actions)
		{
			astra::inspector card;
			const auto speech_name = c->SpeechCharacterName.ToString();
			card.tag = ron::team_name(c);
			card.name = speech_name.empty() ? card.tag : speech_name;

			const float health = c->GetCurrentHealth();
			const float max_health = c->GetMaxHealth();
			card.bar = max_health > 0.f ? std::clamp(health / max_health, 0.f, 1.f) : -1.f;

			const auto status = c->CharacterHealth ? c->CharacterHealth->HealthStatus : SDK::EPlayerHealthStatus::HS_NotAvailable;
			const char* complying = c->bIsFakeSurrender ? "Faking it" : (c->IsComplying() ? "Yes" : "No");
			const char* hostage = c->IsTakingHostage() ? "Holding one" : (c->IsBeingTakenHostage() ? "Being held" : "No");

			card.rows = {
				{ "Health", std::format("{:.0f} / {:.0f}", health, max_health) },
				{ "Status", ron::health_status_name(status) },
				{ "Stress", std::format("{:.0f}", c->Stress) },
				{ "Complying", complying },
				{ "Armed", mission::is_armed(c) ? "Yes" : "No" },
				{ "Explosive vest", c->IsWearingExplosiveVest() ? "Yes" : "No" },
				{ "Hostage", hostage },
				{ "Distance", std::format("{:.0f} m", player::get_player_coords().GetDistanceToInMeters(c->K2_GetActorLocation())) },
			};

			if (ron::is_active(c))
			{
				card.actions.push_back("Arrest");
				actions.push_back(target_action::arrest);
				if (c->IsA(SDK::ASuspectCharacter::StaticClass()) && !c->IsSurrendered())
				{
					card.actions.push_back("Surrender");
					actions.push_back(target_action::surrender);
				}
				if (mission::is_armed(c))
				{
					card.actions.push_back("Disarm");
					actions.push_back(target_action::disarm);
				}
				card.actions.push_back("Knock Out");
				actions.push_back(target_action::knockout);
			}

			card.hint = ron::is_host() ? "Open the menu to act on this target." : "Actions need single player or the host.";
			return card;
		}

		astra::inspector door_card(SDK::ADoor* door, std::vector<target_action>& actions)
		{
			astra::inspector card;
			const bool locked = door->IsLocked();
			card.name = "Door";
			card.tag = locked ? "Locked" : "Unlocked";

			auto trap = door->GetAttachedTrap();
			const bool live_trap = unreal_engine::is_valid_actor(trap) && trap->TrapStatus == SDK::ETrapState::TS_Live;
			const bool wedged = door->GetAttachedWedge() != nullptr;

			card.rows = {
				{ "State", door->IsDoorBroken() ? "Broken" : (door->IsOpen() ? "Open" : "Closed") },
				{ "Trap", live_trap ? ron::trap_type_name(trap->TrapType) : (trap ? "Disarmed" : "None") },
				{ "Wedge", wedged ? "Yes" : "No" },
			};

			card.actions.push_back(locked ? "Unlock" : "Lock");
			actions.push_back(locked ? target_action::unlock : target_action::lock);
			if (live_trap)
			{
				card.actions.push_back("Disarm Trap");
				actions.push_back(target_action::disarm_trap);
			}
			if (wedged)
			{
				card.actions.push_back("Remove Wedge");
				actions.push_back(target_action::remove_wedge);
			}
			card.actions.push_back("Break");
			actions.push_back(target_action::break_door);
			card.actions.push_back("Blow Up");
			actions.push_back(target_action::blow_up_door);

			card.hint = ron::is_host() ? "Open the menu to act on this door." : "Actions need single player or the host.";
			return card;
		}

		void update_inspector()
		{
			std::optional<astra::inspector> card;
			std::vector<target_action> actions;
			weak_object<SDK::ACyberneticCharacter> character;
			weak_object<SDK::ADoor> door;

			auto player = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			if (player && controller)
			{
				if (auto looked_at = door_under_crosshair(controller, player))
				{
					door = looked_at;
					card = door_card(looked_at, actions);
				}
				else if (auto target = character_under_crosshair(controller))
				{
					character = target;
					card = character_card(target, actions);
				}
			}

			std::lock_guard lock(g_shared.mutex);
			g_shared.inspector = std::move(card);
			g_shared.actions = std::move(actions);
			g_shared.character = character;
			g_shared.door = door;
		}

		void update_collectibles()
		{
			std::vector<collectible_entry> found;
			const auto origin = player::get_player_coords();

			if (auto world = SDK::UWorld::GetWorld())
			{
				for (auto level : world->Levels)
				{
					if (!level)
						continue;

					for (auto actor : level->Actors)
					{
						if (!unreal_engine::is_valid_actor(actor) || !actor->IsA(SDK::ACollectable::StaticClass()))
							continue;

						auto collectible = static_cast<SDK::ACollectable*>(actor);
						auto name = ron::text(collectible->ItemName);
						const auto location = collectible->K2_GetActorLocation();
						found.push_back({ name.empty() ? collectible->GetName() : name, location, static_cast<float>(origin.GetDistanceToInMeters(location)) });
					}
				}
			}

			std::ranges::sort(found, {}, &collectible_entry::distance);

			std::lock_guard lock(g_shared.mutex);
			g_shared.collectibles = std::move(found);
		}

		void tick()
		{
			using clock = std::chrono::steady_clock;
			static auto last_mission = clock::time_point{};
			static auto last_inspector = clock::time_point{};
			static auto last_collectibles = clock::time_point{};
			const auto now = clock::now();

			auto gs = ron::game_state();
			if (!gs || !unreal_engine::get_character())
			{
				std::lock_guard lock(g_shared.mutex);
				g_shared.overlay.clear();
				g_shared.inspector.reset();
				g_shared.collectibles.clear();
				return;
			}

			if (now - last_mission > 250ms)
			{
				last_mission = now;
				check_objectives(gs);

				auto rows = _mission_overlay.get_state() ? build_overlay(gs) : std::vector<astra::overlay_row>{};
				std::lock_guard lock(g_shared.mutex);
				g_shared.overlay = std::move(rows);
			}

			if (now - last_inspector > 100ms)
			{
				last_inspector = now;
				if (_target_inspector.get_state())
				{
					update_inspector();
				}
				else
				{
					std::lock_guard lock(g_shared.mutex);
					g_shared.inspector.reset();
				}
			}

			if (_collectibles_window.get_state() && now - last_collectibles > 1s)
			{
				last_collectibles = now;
				update_collectibles();
			}
		}

		// A fault reading the mission skips this read instead of ending the worker.
		void tick_guarded()
		{
			TRY_CLAUSE
			{
				tick();
			}
			EXCEPT_CLAUSE
		}
	}

	task<void> mission_event::run()
	{
		LOG(INFO) << "Mission Event Registered";

		while (g_running)
		{
			tick_guarded();
			co_await this_task::wait();
		}

		co_return;
	}

	std::vector<astra::overlay_row> mission_event::overlay()
	{
		std::lock_guard lock(g_shared.mutex);
		return g_shared.overlay;
	}

	std::optional<astra::inspector> mission_event::inspector()
	{
		std::lock_guard lock(g_shared.mutex);
		return g_shared.inspector;
	}

	std::vector<collectible_entry> mission_event::collectibles()
	{
		std::lock_guard lock(g_shared.mutex);
		return g_shared.collectibles;
	}

	namespace
	{
		// player is who acts: you, or as the host the client that asked.
		void act_on(target_action action, SDK::ACyberneticCharacter* character, SDK::ADoor* door, SDK::APlayerCharacter* player)
		{
			switch (action)
			{
			case target_action::arrest: mission::arrest(character); break;
			case target_action::surrender: mission::surrender(character); break;
			case target_action::disarm: mission::disarm(character); break;
			case target_action::knockout: mission::knockout(character); break;
			case target_action::unlock: mission::set_door_locked(door, false); break;
			case target_action::lock: mission::set_door_locked(door, true); break;
			case target_action::disarm_trap: mission::disarm_door_trap(door); break;
			case target_action::remove_wedge:
				if (door)
					door->RemoveWedges();
				break;
			// As if the host had kicked it down or blown it with C2.
			case target_action::break_door:
				if (door)
					door->BreakDoor(false, player);
				break;
			case target_action::blow_up_door:
				if (door && player)
					door->ExplodeDoor(player, player, false);
				break;
			}
		}

		// As the host: the inspector action a client chose, on what stands nearest to where the client saw its target,
		// within 3 m; the client sends action:x:y:z.
		host_link::client_action _inspector_for_clients("inspector", "Target Inspector", [](SDK::APlayerCharacter* player, const std::string& args) {
			int index = -1;
			SDK::FVector at{};
			if (std::sscanf(args.c_str(), "%d:%lf:%lf:%lf", &index, &at.X, &at.Y, &at.Z) != 4 || index < 0
			    || index > static_cast<int>(target_action::disarm))
				return false;

			const auto action = static_cast<target_action>(index);
			const bool on_door = action >= target_action::unlock && action <= target_action::blow_up_door;
			SDK::ACyberneticCharacter* character = nullptr;
			SDK::ADoor* door = nullptr;
			double nearest = 3.0;

			if (on_door)
			{
				if (auto gs = ron::game_state())
				{
					for (auto candidate : gs->AllDoors)
					{
						if (!unreal_engine::is_valid_actor(candidate))
							continue;
						if (const double distance = at.GetDistanceToInMeters(candidate->K2_GetActorLocation()); distance < nearest)
						{
							nearest = distance;
							door = candidate;
						}
					}
				}
			}
			else
			{
				ron::for_each_ai([&](SDK::ACyberneticCharacter* candidate) {
					if (const double distance = at.GetDistanceToInMeters(candidate->K2_GetActorLocation()); distance < nearest)
					{
						nearest = distance;
						character = candidate;
					}
				});
			}

			if (!character && !door)
				return false;

			act_on(action, character, door, player);
			return true;
		});
	}

	void mission_event::inspector_action(int index)
	{
		target_action action;
		weak_object<SDK::ACyberneticCharacter> character;
		weak_object<SDK::ADoor> door;
		{
			std::lock_guard lock(g_shared.mutex);
			if (index < 0 || index >= static_cast<int>(g_shared.actions.size()))
				return;

			action = g_shared.actions[index];
			character = g_shared.character;
			door = g_shared.door;
		}

		async_run([action, character, door] {
			// As a client, the host does it when it runs this mod, finding the same target by where it stands.
			if (!ron::is_host())
			{
				auto target = character.get() ? static_cast<SDK::AActor*>(character.get()) : static_cast<SDK::AActor*>(door.get());
				if (target)
				{
					const auto at = target->K2_GetActorLocation();
					host_link::run("inspector", "Target Inspector", std::format("{}:{}:{}:{}", static_cast<int>(action), at.X, at.Y, at.Z));
				}
				return;
			}

			act_on(action, character.get(), door.get(), unreal_engine::get_character());
		});
	}

	void mission_event::teleport(const SDK::FVector& location)
	{
		async_run([location] {
			auto destination = location;
			destination.Z += 100.f;
			player::set_player_coords(destination, player::get_player_rotation());
			notification::success("Teleport", "Teleported to the collectible.");
		});
	}
}
