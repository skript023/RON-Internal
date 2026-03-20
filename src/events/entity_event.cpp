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
		const char* label;
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

	static const char* find_name(std::string_view key, const auto& table)
	{
		for (const auto& e : table)
		{
			if (key.contains(e.key))
				return e.value.data();
		}

		return nullptr;
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

	void entity_event::run()
	{
		LOG(INFO) << "Entity Event Registered";

		while (g_running)
		{
			auto& back = g_esp_data.back(); back.clear();
			auto world = SDK::UWorld::GetWorld(); if (!world) continue;
			auto ch = unreal_engine::get_character(); if (!ch) continue;
			auto c = unreal_engine::get_player_controller(); if (!c) continue;
			auto level = world->PersistentLevel; if (!level) continue;

			if (auto actors = level->Actors; actors.Num() > 0)
			{
				for (size_t i = 0; i < actors.Num(); i++)
				{
					if (!actors.IsValidIndex(i)) continue;

					auto actor = actors[i];

					if (!actor) continue;

					auto location = actor->K2_GetActorLocation();

					float distance = player::get_player_coords().GetDistanceToInMeters(location);

					auto name = actor->GetFullName();
					//LOG(INFO) << actor->GetName();
					auto result = filter_actor(actor->GetName());

					if (!result) continue;

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
						}
					}
					
					SDK::FVector2D screen;
					if (!c->ProjectWorldLocationToScreen(location, &screen, true)) continue;

					char buffer[128];
					snprintf(
						buffer,
						sizeof(buffer),
						"%s [%s] [%.2f]m",
						result->label,
						get_status(status),
						distance
					);

					esp_data actor_data;
					actor_data.actor = target_pawn;
					actor_data.location = location;
					actor_data.screen = screen;
					actor_data.rotation = actor->K2_GetActorRotation();
					actor_data.display_classname = name;
					actor_data.display_text = buffer;
					actor_data.distance = distance;
					actor_data.status = status;
					actor_data.color = result->color;
					actor_data.enemy = result->enemy;

					back.push_back(actor_data);
				}
			}

			g_esp_data.publish();
		}

		std::this_thread::sleep_for(30ms);
	}
}