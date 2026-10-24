#pragma once
#include "unreal_engine_utility.hpp"
#include "weak_object.hpp"

#include <astra/host/notification.hpp>

// Ready Or Not helpers the features, the ESP worker, and the mission panels share. Call them from the game thread.
namespace big::ron
{
	inline SDK::AReadyOrNotGameState* game_state()
	{
		auto world = SDK::UWorld::GetWorld();
		if (!world || !world->GameState || !world->GameState->IsA(SDK::AReadyOrNotGameState::StaticClass()))
			return nullptr;

		return static_cast<SDK::AReadyOrNotGameState*>(world->GameState);
	}

	inline SDK::AReadyOrNotPlayerController* controller()
	{
		auto controller = unreal_engine::get_player_controller();
		if (!controller || !controller->IsA(SDK::AReadyOrNotPlayerController::StaticClass()))
			return nullptr;

		return static_cast<SDK::AReadyOrNotPlayerController*>(controller);
	}

	inline bool is_in_mission()
	{
		auto world = SDK::UWorld::GetWorld();
		auto gs = game_state();
		auto controller = unreal_engine::get_player_controller();
		auto character = unreal_engine::get_character();

		return world && world->PersistentLevel && gs && controller && unreal_engine::is_valid_actor(character)
		    && gs->MatchState == SDK::EMatchState::MS_Playing;
	}

	// Single player, or the host of the lobby: the game only accepts mission changes made here.
	inline bool is_host()
	{
		auto controller = unreal_engine::get_player_controller();
		return controller && controller->HasAuthority();
	}

	inline bool require_host(const std::string& feature)
	{
		if (is_host())
			return true;

		notification::warning(feature, "Only works in single player or as the lobby host.");
		return false;
	}

	// Lives as long as the game instance, so it is looked up again only after the old one is gone.
	inline SDK::UReadyOrNotDebugSubsystem* debug_subsystem()
	{
		static weak_object<SDK::UReadyOrNotDebugSubsystem> cached;
		if (auto subsystem = cached.get())
			return subsystem;

		auto world = SDK::UWorld::GetWorld();
		if (!world)
			return nullptr;

		auto subsystem = static_cast<SDK::UReadyOrNotDebugSubsystem*>(
		    SDK::USubsystemBlueprintLibrary::GetGameInstanceSubsystem(world, SDK::UReadyOrNotDebugSubsystem::StaticClass()));
		cached = subsystem;
		return subsystem;
	}

	inline std::string text(const SDK::FText& value)
	{
		return value.TextData ? value.ToString() : std::string{};
	}

	inline const char* health_status_name(SDK::EPlayerHealthStatus status)
	{
		using enum SDK::EPlayerHealthStatus;
		switch (status)
		{
		case HS_Healthy: return "Healthy";
		case HS_Injured: return "Injured";
		case HS_Downed: return "Downed";
		case HS_Incapacitated: return "Incapacitated";
		case HS_Dead: return "Dead";
		case HS_Arrested: return "Arrested";
		default: return "Unknown";
		}
	}

	inline const char* trap_type_name(SDK::ETrapType type)
	{
		using enum SDK::ETrapType;
		switch (type)
		{
		case Alarm: return "Alarm Trap";
		case Flashbang: return "Flashbang Trap";
		case Explosive: return "Explosive Trap";
		case ToxicGas: return "Gas Trap";
		default: return "Trap";
		}
	}

	inline const char* team_name(const SDK::AReadyOrNotCharacter* character)
	{
		if (character->IsA(SDK::ASuspectCharacter::StaticClass()))
			return "Suspect";
		if (character->IsA(SDK::ACivilianCharacter::StaticClass()))
			return "Civilian";
		if (character->IsA(SDK::ASWATCharacter::StaticClass()))
			return "SWAT";
		if (character->IsA(SDK::APlayerCharacter::StaticClass()))
			return "Player";
		return "Unknown";
	}

	// An AI that still matters to the mission: not dead, not unconscious, and not in cuffs.
	inline bool is_active(SDK::AReadyOrNotCharacter* character)
	{
		return character && !character->IsDeadOrUnconscious() && !character->IsArrested();
	}

	template <typename F>
	inline void for_each_ai(F&& callback)
	{
		auto gs = game_state();
		if (!gs)
			return;

		auto& characters = gs->AllAICharacters;
		for (int i = 0; i < characters.Num(); ++i)
		{
			if (!characters.IsValidIndex(i))
				continue;
			if (auto character = characters[i]; unreal_engine::is_valid_actor(character))
				callback(character);
		}
	}
}
