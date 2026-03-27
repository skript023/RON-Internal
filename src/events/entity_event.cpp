#include "script.hpp"
#include "entity_event.hpp"
#include "utility/player.hpp"

namespace big
{
	struct name_map_entry
	{
		std::string_view key;
		std::string_view value;
	};

	struct esp_result
	{
		std::string label;
		Color color;
		bool enemy;
	};

	// Enemy
	constexpr std::array<name_map_entry, 1> enemy{ {
		{ "CyberneticsSuspect_V2_C", "Suspect" }
	} };

	// Friend
	constexpr std::array<name_map_entry, 2> friendlies{ {
		{ "CyberneticsSwat_V2_C", "Swat" },
		{ "Cybernetics_Civilian_V2_C", "Civilian" }
	} };

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

	// Traps
	constexpr std::array<name_map_entry, 3> traps{ {
		{ "BP_AttachedGrenadeTrap_C", "Grenade Trap" },
		{ "BP_AttachedFlashbangTrap_C", "Flashbang Trap" },
		{ "BP_AttachedAlarmTrap_C", "Alarm Trap" }
	} };

	// Objectives
	constexpr std::array<name_map_entry, 34> objectives{ {
		{ "Objective_Hotel_Evidence_Weapon_02_C", "Evidence Weapon" },
		{ "Objective_Hotel_Evidence_Weapon_01_C", "Evidence Weapon" },
		{ "Objective_Hotel_RescueStrasky_C", "Rescue Strasky" },
		{ "Objective_Hotel_RescueBrady_C", "Rescue Brady" },
		{ "Objective_Hotel_ArrestJohnnyR_C", "Arrest Johnny" },
		{ "Objective_Gas_Rescue_CrystalLeighton_C", "Rescue Crystal" },
		{ "Objective_Gas_Report_MudasirVarma_C", "Report Mudasir" },
		{ "Objective_Gas_Rescue_SharlaLeighton_C", "Rescue Sharla" },
		{ "Objective_Zoo_Neutralize_TaggedSuspect_C", "Neutralize Suspect" },
		{ "Objective_Zoo_Rescue_Civilians_C", "Rescue Civilian" },
		{ "Objective_Valley_Seize_Laptop_C", "Seize Laptop" },
		{ "Objective_Valley_Seize_Hard_Drive_01_C", "Seize Harddrive" },
		{ "Objective_Valley_ArrestAmos_C", "Arrest Amos" },
		{ "Objective_Valley_Seize_Hard_Drive_02_C", "Seize Harddrive" },
		{ "Objective_Port_Dossier_C", "Dossier" },
		{ "Objective_Port_ArrestSuspects_C", "Arrest Suspect" },
		{ "Objective_Port_ArrestAuctioneer_C", "Arrest Auctioneer" },
		{ "Objective_Port_RescueSlaves_C", "Rescue Slave" },
		{ "Objective_Importer_ArrestJackAdams_C", "Arrest Jack" },
		{ "Objective_Importer_ArrestMariaLopez_C", "Arrest Maria" },
		{ "Objective_Importer_RescueEugeneGomez_C", "Rescue Eugene" },
		{ "Objective_FastFood_RescueOfficer_C", "Rescue Officer" },
		{ "Objective_FastFood_StaffRoster_C", "StaffRoster" },
		{ "Objective_FastFood_Neutralize_Gunman_C", "Neutralize Gunman" },
		{ "Objective_Ridgeline_ArrestGerardScott_C", "Arrest Gerard" },
		{ "Objective_Ridgeline_ArrestGerardAccomplice_C", "Arrest Accomplice" },
		{ "Objective_Shipyard_ExtractEvidence_C", "Extract Evidence" },
		{ "Objective_Shipyard_Exfil_C", "Exfil" },
		{ "Objective_Shipyard_FindEvidence_C", "Evidence" },
		{ "Objective_Shipyard_FindClues_C", "Clue" },
		{ "Objective_Penthouse_LocateSailor_C", "Sailor" },
		{ "Objective_Penthouse_LocateLaurie_C", "Laurie" }
	} };

	static bool is_skeleton_variant_2(SDK::USkeletalMeshComponent* mesh)
	{
		if (!mesh) return false;

		auto name = mesh->GetBoneName(45).ToString();
		return name == "Head";
	}

	static const char* find_name(std::string_view key, const auto& table)
	{
		for (const auto& e : table)
		{
			if (key.contains(e.key))
				return e.value.data();
		}

		return nullptr;
	}

	static std::optional<esp_result> filter_actor(SDK::AActor* actor)
	{
		if (!actor) return std::nullopt;

		if (actor->IsA(SDK::ATrapActor::StaticClass()))
		{
			if (auto trap = static_cast<SDK::ATrapActor*>(actor); trap)
			{
				switch (trap->TrapType)
				{
				case SDK::ETrapType::Explosive:
					return esp_result{ "Explosive Trap", {255, 120, 0, 255}, false };

				case SDK::ETrapType::Flashbang:
					return esp_result{ "Flashbang Trap", {255, 120, 0, 255}, false };

				case SDK::ETrapType::Alarm:
					return esp_result{ "Alarm Trap", {255, 120, 0, 255}, false };

				case SDK::ETrapType::ToxicGas:
					return esp_result{ "Gas Trap", {255, 120, 0, 255}, false };

				case SDK::ETrapType::Unknown:
					return esp_result{ "Unknown Trap", {255, 120, 0, 255}, false };

				case SDK::ETrapType::ETrapType_MAX:
					return esp_result{ "Invalid Trap", {255, 120, 0, 255}, false };

				default:
					return esp_result{ "Unknown Trap", {255, 120, 0, 255}, false };
				}
			}
		}

		if (actor->IsA(SDK::ACivilianCharacter::StaticClass()))
		{
			if (auto civilian = static_cast<SDK::ACivilianCharacter*>(actor); civilian)
			{
				return esp_result{ "Civilian", {0, 255, 0, 255}, false };
			}
		}

		if (actor->IsA(SDK::ASWATCharacter::StaticClass()))
		{
			if (auto swat = static_cast<SDK::ASWATCharacter*>(actor); swat)
			{
				return esp_result{ "Swat", {0, 255, 0, 255}, false };
			}
		}

		if (actor->IsA(SDK::ASuspectCharacter::StaticClass()))
		{
			if (auto suspect = static_cast<SDK::ASuspectCharacter*>(actor); suspect)
			{
				return esp_result{ "Suspect", {255, 0, 0, 255}, true };
			}
		}

		if (actor->IsA(SDK::ABaseMagazineWeapon::StaticClass()))
		{
			if (auto wep = static_cast<SDK::ABaseMagazineWeapon*>(actor); wep)
			{
				auto name = wep->GetName();
				for (const auto& e : weapons)
				{
					if (name.contains(e.key))
						return esp_result{ e.value.data(), {255, 255, 0, 255}, false };
				}
			}
		}

		if (actor->IsA(SDK::AReportableActor::StaticClass()))
		{
			if (auto obj = static_cast<SDK::AReportableActor*>(actor); obj)
			{
				Color color = obj->bReportableEnabled ? Color{ 150, 150, 150, 255 } : Color{ 0, 200, 255, 255 };
				return esp_result{ obj->ReportableName.ToString(), color, false };
			}
		}

		return std::nullopt;
	}

	static std::optional<esp_result> filter_actor(std::string_view name)
	{
		if (auto r = find_name(name, enemy))
			return esp_result{ r, {255, 0, 0, 255}, true };

		if (auto r = find_name(name, friendlies))
			return esp_result{ r, {0, 255, 0, 255}, false };

		if (auto r = find_name(name, weapons))
			return esp_result{ r, {255, 255, 0, 255}, false };

		if (auto r = find_name(name, traps))
			return esp_result{ r, {255, 120, 0, 255}, false };

		if (auto r = find_name(name, objectives))
			return esp_result{ r, {0, 200, 255, 255}, false };

		return std::nullopt;
	}

	static const char* get_status(SDK::EPlayerHealthStatus status)
	{
		using namespace SDK;

		switch (status)
		{
		case EPlayerHealthStatus::HS_Arrested:
			return "Arrested";
		case EPlayerHealthStatus::HS_Dead:
			return "Dead";
		case EPlayerHealthStatus::HS_Downed:
			return "Downed";
		case EPlayerHealthStatus::HS_Healthy:
			return "Healthy";
		case EPlayerHealthStatus::HS_Incapacitated:
			return "Incapacitated";
		case EPlayerHealthStatus::HS_Injured:
			return "Injured";
		case EPlayerHealthStatus::HS_NotAvailable:
			return "Not Available";
		case EPlayerHealthStatus::HS_MAX:
			return "MAX";
		}

		return "None";
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

	void entity_event::run()
	{
		if (g_pointers->m_tls_idx)
		{
			auto value = TlsGetValue(*g_pointers->m_tls_idx);
			TlsSetValue(*g_pointers->m_tls_idx, value);

			LOG(INFO) << "TLS set same index as main thread";
		}

		LOG(INFO) << "Entity Event Registered";

		while (g_running)
		{
			auto& back = g_esp_data.back(); back.clear();
			auto world = SDK::UWorld::GetWorld(); if (!world) continue;
			auto ch = unreal_engine::get_character(); if (!ch) continue;
			auto c = unreal_engine::get_player_controller(); if (!c) continue;
			auto level = world->PersistentLevel; if (!level) continue;

			if (auto gs = world->GameState; gs)
			{
				if (auto ron_gs = static_cast<SDK::AReadyOrNotGameState*>(gs))
				{
					auto objs = ron_gs->AllReportableActors;
					auto missions = ron_gs->MissionObjectives;

					for (int i = 0; i < objs.Num(); ++i)
					{
						if (!objs.IsValidIndex(i)) continue;

						auto obj = objs[i];
						if (!obj) continue;

						auto location = obj->K2_GetActorLocation();

						float distance = player::get_player_coords()
							.GetDistanceToInMeters(location);

						SDK::FVector2D screen;
						if (!c->ProjectWorldLocationToScreen(location, &screen, true))
							continue;

						// 🔥 STRING AMAN (std::string)
						auto name = obj->ReportableName.ToString();

						char buffer[128];
						snprintf(
							buffer,
							sizeof(buffer),
							"%s [%.2f]m",
							name.c_str(),
							distance
						);

						esp_data actor_data;
						actor_data.location = location;
						actor_data.screen = screen;
						actor_data.display_classname = "Objective";
						actor_data.display_text = buffer;
						actor_data.distance = distance;
						actor_data.status = SDK::EPlayerHealthStatus::HS_NotAvailable;
						actor_data.color = obj->bReportableEnabled
							? Color{ 150,150,150,255 }   // gray
						: Color{ 0, 200, 255, 255 };      // green
						actor_data.enemy = false;

						back.push_back(actor_data);
					}

					for (size_t i = 0; i < missions.Num(); i++)
					{
						if (!missions.IsValidIndex(i)) continue;

						auto mission = missions[i];
						if (!mission) continue;

						auto result = filter_actor(mission);

						auto location = mission->K2_GetActorLocation();

						float distance = player::get_player_coords()
							.GetDistanceToInMeters(location);

						SDK::FVector2D screen;
						if (!c->ProjectWorldLocationToScreen(location, &screen, true))
							continue;

						char buffer[128];
						snprintf(
							buffer,
							sizeof(buffer),
							"%s [%.2f]m",
							result->label.c_str(),
							distance
						);

						esp_data actor_data;
						actor_data.location = location;
						actor_data.screen = screen;
						actor_data.display_classname = "Objective";
						actor_data.display_text = buffer;
						actor_data.distance = distance;
						actor_data.status = SDK::EPlayerHealthStatus::HS_NotAvailable;
						actor_data.color = result->color;
						actor_data.enemy = false;

						back.push_back(actor_data);
					}
				}
			}

			if (auto actors = level->Actors; actors.Num() > 0)
			{
				for (size_t i = 0; i < actors.Num(); i++)
				{
					if (!actors.IsValidIndex(i)) continue;

					auto actor = actors[i];

					if (!actor || (uintptr_t)actor < 0x10000) continue;

					auto result = filter_actor(actor);

					if (!result) continue;

					auto location = actor->K2_GetActorLocation();

					float distance = player::get_player_coords().GetDistanceToInMeters(location);

					auto name = actor->GetFullName();

					SDK::AReadyOrNotCharacter* target_pawn = nullptr;

					SDK::EPlayerHealthStatus status = SDK::EPlayerHealthStatus::HS_NotAvailable;

					if (actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
					{
						if (target_pawn = static_cast<SDK::AReadyOrNotCharacter*>(actor))
						{
							if (auto health = target_pawn->CharacterHealth)
							{
								if (health)
									status = health->HealthStatus;
							}

							location = unreal_engine::get_location_bone(target_pawn, EBonesIndex::Root);
						}
					}
					
					SDK::FVector2D screen;
					if (!c->ProjectWorldLocationToScreen(location, &screen, false)) continue;

					char buffer[128];

					if (target_pawn)
					{
						snprintf(
							buffer,
							sizeof(buffer),
							"%s [%s] [%.2f]m",
							result->label.c_str(),
							get_status(status),
							distance
						);
					}
					else
					{
						snprintf(
							buffer,
							sizeof(buffer),
							"%s [%.2f]m",
							result->label.c_str(),
							distance
						);
					}

					esp_data actor_data;
					actor_data.location = location;
					actor_data.screen = screen;
					actor_data.display_classname = name;
					actor_data.display_text = buffer;
					actor_data.distance = distance;
					actor_data.status = status;
					actor_data.color = result->color;
					actor_data.enemy = result->enemy;
					actor_data.has_skeleton = build_skeleton(target_pawn, c, actor_data.skeleton);

					actor_data.has_box = build_box(
						target_pawn,
						c,
						actor_data.box_x,
						actor_data.box_y,
						actor_data.box_w,
						actor_data.box_h
					);

					actor_data.has_box_3d = build_box_3d(target_pawn, c, actor_data.box_3d);

					back.push_back(actor_data);
				}
			}

			g_esp_data.publish();
		}

		std::this_thread::sleep_for(30ms);
	}
}