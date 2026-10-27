#include "mission.hpp"
#include "script.hpp"
#include "commands/command.hpp"
#include "commands/looped_command.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big::mission
{
	static bool is_suspect_or_civilian(SDK::ACyberneticCharacter* character)
	{
		return character->DefaultTeam == SDK::ETeamType::TT_SUSPECT || character->DefaultTeam == SDK::ETeamType::TT_CIVILIAN;
	}

	static std::vector<SDK::ACyberneticCharacter*> collect_ai(bool (*keep)(SDK::ACyberneticCharacter*))
	{
		// Gathered first: arresting or reporting can change the game's own lists while they are walked.
		std::vector<SDK::ACyberneticCharacter*> found;
		ron::for_each_ai([&](SDK::ACyberneticCharacter* character) {
			if (keep(character))
				found.push_back(character);
		});
		return found;
	}

	void arrest(SDK::ACyberneticCharacter* character)
	{
		auto player = unreal_engine::get_character();
		if (!player || !character || character->IsArrested() || character->IsDeadNotUnconscious())
			return;

		character->ArrestComplete(player, nullptr);
		player->Server_ReportToTOC(character, false, false);
	}

	void surrender(SDK::ACyberneticCharacter* character)
	{
		if (ron::is_active(character) && !character->IsSurrendered())
			character->Surrender();
	}

	void knockout(SDK::ACyberneticCharacter* character)
	{
		if (ron::is_active(character))
			character->Knockout(30.f, false);
	}

	int arrest_all()
	{
		if (!unreal_engine::get_character())
			return 0;

		auto targets = collect_ai([](SDK::ACyberneticCharacter* c) {
			return is_suspect_or_civilian(c) && !c->IsArrested() && !c->IsDeadNotUnconscious();
		});
		for (auto character : targets)
			arrest(character);
		return static_cast<int>(targets.size());
	}

	int report_all()
	{
		auto player = unreal_engine::get_character();
		auto gs = ron::game_state();
		if (!player || !gs)
			return 0;

		// Anyone arrested, down, or dead, and every reportable item, that TOC has not heard about yet.
		auto targets = collect_ai([](SDK::ACyberneticCharacter* c) {
			return !c->bHasBeenReported && (c->IsArrested() || c->IsDeadOrUnconscious() || c->IsIncapacitated());
		});

		std::vector<SDK::AActor*> reportables;
		for (auto reportable : gs->AllReportableActors)
		{
			if (unreal_engine::is_valid_actor(reportable) && reportable->bReportableEnabled && !reportable->bHasBeenReported)
				reportables.push_back(reportable);
		}

		for (auto character : targets)
			player->Server_ReportToTOC(character, false, false);
		for (auto reportable : reportables)
			player->Server_ReportToTOC(reportable, false, false);

		return static_cast<int>(targets.size() + reportables.size());
	}

	int collect_evidence()
	{
		auto player = unreal_engine::get_character();
		auto gs = ron::game_state();
		if (!player || !gs)
			return 0;

		// PickupEvidence bags a dropped weapon, as the earlier Collect All Evidance used it; given other items and the
		// mission's evidence it faulted inside the game before anything was collected. The mission's own evidence, such
		// as phones or documents, is secured through its evidence state, as the earlier Auto Win did.
		std::vector<SDK::ABaseItem*> weapons;
		for (auto item : gs->AllItems)
		{
			if (unreal_engine::is_valid_actor(item) && item->IsA(SDK::ABaseWeapon::StaticClass()) && item->EvidenceComponent && item->EvidenceComponent->CanBeCollected())
				weapons.push_back(item);
		}
		std::vector<SDK::AEvidenceActor*> mission_evidence;
		for (auto actor : gs->AllEvidenceActors)
		{
			if (unreal_engine::is_valid_actor(actor) && actor->EvidenceComponent && actor->EvidenceComponent->CanBeCollected())
				mission_evidence.push_back(actor);
		}

		for (auto weapon : weapons)
			player->PickupEvidence(weapon);
		for (auto evidence : mission_evidence)
			evidence->OnEvidenceStateChanged(SDK::EEvidenceActorState::Collected);

		return static_cast<int>(weapons.size() + mission_evidence.size());
	}

	int surrender_suspects()
	{
		auto targets = collect_ai([](SDK::ACyberneticCharacter* c) {
			return c->IsA(SDK::ASuspectCharacter::StaticClass()) && ron::is_active(c) && !c->IsSurrendered();
		});
		for (auto character : targets)
			surrender(character);
		return static_cast<int>(targets.size());
	}

	int knockout_suspects()
	{
		auto targets = collect_ai([](SDK::ACyberneticCharacter* c) {
			return c->IsA(SDK::ASuspectCharacter::StaticClass()) && ron::is_active(c);
		});
		for (auto character : targets)
			knockout(character);
		return static_cast<int>(targets.size());
	}

	void set_door_locked(SDK::ADoor* door, bool locked)
	{
		if (!door)
			return;

		// SetLocked tells every player; the flag keeps the door usable here even if the call does nothing.
		door->SetLocked(locked);
		door->bLocked = locked;
	}

	int unlock_doors()
	{
		auto gs = ron::game_state();
		if (!gs)
			return 0;

		int count = 0;
		for (auto door : gs->AllDoors)
		{
			if (!unreal_engine::is_valid_actor(door) || !door->IsLocked())
				continue;

			set_door_locked(door, false);
			++count;
		}
		return count;
	}

	// The game will not open a locked door, so it is unlocked first. A door with a live trap stays shut rather than set
	// the trap off, and one that cannot swing at all is left alone.
	int open_doors()
	{
		auto gs = ron::game_state();
		auto character = unreal_engine::get_character();
		if (!gs || !character)
			return 0;

		int count = 0;
		for (auto door : gs->AllDoors)
		{
			if (!unreal_engine::is_valid_actor(door) || door->GetMaxOpenAmount() <= 0.f || std::abs(door->GetOpenAmount()) > 1.f)
				continue;
			if (auto trap = door->GetAttachedTrap(); trap && trap->TrapStatus == SDK::ETrapState::TS_Live)
				continue;

			if (door->IsLocked())
				set_door_locked(door, false);
			// The angle it swings to, or 0 when it does not open.
			if (door->OpenDoor(character, false, true, true) != 0.f)
				++count;
		}
		return count;
	}

	static void disarm(SDK::ATrapActor* trap)
	{
		if (trap && trap->TrapStatus == SDK::ETrapState::TS_Live)
			trap->Server_OnTrapDisarmed();
	}

	void disarm_door_trap(SDK::ADoor* door)
	{
		if (door)
			disarm(door->GetAttachedTrap());
	}

	int disarm_traps()
	{
		auto world = SDK::UWorld::GetWorld();
		if (!world)
			return 0;

		// Door traps and tripwires alike, in every loaded level.
		std::vector<SDK::ATrapActor*> traps;
		for (auto level : world->Levels)
		{
			if (!level)
				continue;

			for (auto actor : level->Actors)
			{
				if (!unreal_engine::is_valid_actor(actor) || !actor->IsA(SDK::ATrapActor::StaticClass()))
					continue;

				auto trap = static_cast<SDK::ATrapActor*>(actor);
				if (trap->TrapStatus == SDK::ETrapState::TS_Live)
					traps.push_back(trap);
			}
		}

		for (auto trap : traps)
			disarm(trap);
		return static_cast<int>(traps.size());
	}

	int remove_wedges()
	{
		auto gs = ron::game_state();
		if (!gs)
			return 0;

		int count = 0;
		for (auto door : gs->AllDoors)
		{
			if (unreal_engine::is_valid_actor(door) && door->GetAttachedWedge())
			{
				door->RemoveWedges();
				++count;
			}
		}
		return count;
	}

	bool teleport_to_next_ai()
	{
		auto player = unreal_engine::get_character();
		if (!player)
			return false;

		const auto origin = player::get_player_coords();
		SDK::ACyberneticCharacter* nearest = nullptr;
		float nearest_distance = std::numeric_limits<float>::max();

		// The closest suspect still in the fight, then the closest civilian still to secure.
		for (auto team : { SDK::ETeamType::TT_SUSPECT, SDK::ETeamType::TT_CIVILIAN })
		{
			ron::for_each_ai([&](SDK::ACyberneticCharacter* c) {
				if (c->DefaultTeam != team || !ron::is_active(c))
					return;

				const float distance = origin.GetDistanceToInMeters(c->K2_GetActorLocation());
				if (distance < nearest_distance)
				{
					nearest = c;
					nearest_distance = distance;
				}
			});

			if (nearest)
				break;
		}

		if (!nearest)
			return false;

		auto location = nearest->K2_GetActorLocation();
		location.Z += 100.f;
		player::set_player_coords(location, player::get_player_rotation());
		return true;
	}
}

namespace big
{
	// A menu row that runs one mission action as the host and reports what it did.
	class mission_action : public command
	{
	public:
		using action_t = std::function<int()>;

		mission_action(std::string name, std::string label, std::string description, std::string done, action_t action) :
		    command(std::move(name), std::move(label), std::move(description), 0),
		    m_done(std::move(done)),
		    m_action(std::move(action))
		{
		}

	protected:
		void on_call() override
		{
			if (!ron::require_host(get_label()))
				return;

			const int count = m_action();
			if (count > 0)
				notification::success(get_label(), std::vformat(m_done, std::make_format_args(count)));
			else
				notification::info(get_label(), "Nothing left to do.");
		}

	private:
		std::string m_done;
		action_t m_action;
	};

	static mission_action _auto_arrest("auto_arrest", "Arrest All", "Cuffs every suspect and civilian and reports them", "{} arrested.", mission::arrest_all);
	static mission_action _report_all("report_all", "Report All", "Reports everyone arrested or down, and every reportable item, to TOC", "{} reported.", mission::report_all);
	static mission_action _collect_evidence("collect_evidence", "Collect All Evidence", "Picks up every dropped weapon and piece of evidence", "{} collected.", mission::collect_evidence);
	static mission_action _surrender_suspects("surrender_suspects", "Surrender Suspects", "Every suspect still fighting gives up", "{} surrendered.", mission::surrender_suspects);
	static mission_action _knockout_suspects("knockout_suspects", "Knock Out Suspects", "Every suspect still fighting is knocked out for 30 seconds", "{} knocked out.", mission::knockout_suspects);
	static mission_action _unlock_doors("unlock_doors", "Unlock All Doors", "Every locked door opens for you and your squad", "{} doors unlocked.", mission::unlock_doors);
	static mission_action _open_doors("open_doors", "Open All Doors", "Swings every closed door open, unlocking locked ones; trapped ones stay shut", "{} doors opened.", mission::open_doors);
	static mission_action _disarm_traps("disarm_traps", "Disarm All Traps", "Disarms every live door trap and tripwire", "{} traps disarmed.", mission::disarm_traps);
	static mission_action _remove_wedges("remove_wedges", "Remove Door Wedges", "Pulls every door wedge", "{} wedges removed.", mission::remove_wedges);
	static mission_action _teleport_next_ai("teleport_next_ai", "Teleport to Next AI", "Moves you to the closest suspect still fighting, or civilian still to secure", "Teleported.", [] {
		return mission::teleport_to_next_ai() ? 1 : 0;
	});
	static mission_action _auto_win("auto_win", "Auto Win", "Arrests everyone, collects all evidence, and reports everything", "{} actions done.", [] {
		return mission::arrest_all() + mission::collect_evidence() + mission::report_all();
	});

	static bool has_given_penalty(const SDK::FScoreData& score)
	{
		for (const auto& penalty : score.Penalties)
		{
			if (penalty.bGiven)
				return true;
		}
		return false;
	}

	// The game gives penalties from its own code, which a ProcessEvent hook does not see, so the host takes each one back
	// through the scoring manager on its next tick; only the popup for it still shows.
	class no_penalty : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto manager = ron::is_host() ? SDK::AScoringManager::Get() : nullptr;
			if (!manager)
				return;

			// Gathered first: taking a penalty back changes the lists being walked.
			std::vector<SDK::UScoringComponent*> penalised;
			for (const auto& group : manager->ObjectiveScoreGroups)
			{
				for (const auto& score : group.Scores)
				{
					if (score.FromScoringComponent && has_given_penalty(score))
						penalised.push_back(score.FromScoringComponent);
				}
			}

			for (auto component : penalised)
				manager->RevokeAllPenalties(component);
		}
	};

	static no_penalty _no_penalty("no_penalty", "No Penalty", "Penalties, such as for unauthorised force, are taken back as soon as they are given (host only)");
}
