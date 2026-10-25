#include "script.hpp"
#include "commands/command.hpp"
#include "commands/float_command.hpp"
#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	using debug_subsystem = SDK::UReadyOrNotDebugSubsystem;

	// A switch the game's own debug subsystem keeps. It calls the game's toggle, and writes the flag directly when the
	// toggle did nothing, such as when a shipping build left its body out. Re-applied every tick, so a new mission
	// keeps it.
	class debug_toggle : public looped_command
	{
	public:
		using getter_t = bool (*)(debug_subsystem*);
		using setter_t = void (*)(debug_subsystem*, bool);
		using toggler_t = void (*)(debug_subsystem*);

		debug_toggle(std::string name, std::string label, std::string description, getter_t get, setter_t set, toggler_t toggle) :
		    looped_command(std::move(name), std::move(label), std::move(description)),
		    m_get(get),
		    m_set(set),
		    m_toggle(toggle)
		{
		}

	protected:
		void on_tick() override
		{
			if (auto subsystem = ron::debug_subsystem(); subsystem && !m_get(subsystem))
				apply(subsystem, true);
		}

		void on_disable() override
		{
			if (auto subsystem = ron::debug_subsystem(); subsystem && m_get(subsystem))
				apply(subsystem, false);
		}

	private:
		void apply(debug_subsystem* subsystem, bool on)
		{
			m_toggle(subsystem);
			if (m_get(subsystem) != on)
				m_set(subsystem, on);
		}

		getter_t m_get;
		setter_t m_set;
		toggler_t m_toggle;
	};

	// The debug subsystem's AI movement and AI targeting switches did nothing in the shipping game, so these two work on
	// the characters themselves.

	// Each AI a feature changed, listed once however often the game undoes the change, so turning it off puts each one
	// back. Anyone the engine freed since, such as on a level change, is dropped.
	static void track(std::vector<weak_object<SDK::ACyberneticCharacter>>& changed, SDK::ACyberneticCharacter* ai)
	{
		std::erase_if(changed, [](const auto& entry) { return !entry; });

		const weak_object<SDK::ACyberneticCharacter> entry(ai);
		if (std::ranges::find(changed, entry) == changed.end())
			changed.push_back(entry);
	}

	// Suspects and civilians stop moving: movement off on each one, back to walking afterwards. AI moves on the host,
	// so only the host freezes it; the squad keeps moving.
	class freeze_ai : public looped_command
	{
		using looped_command::looped_command;

		std::vector<weak_object<SDK::ACyberneticCharacter>> m_frozen;

		void on_tick() override
		{
			if (!ron::is_host())
				return;

			ron::for_each_ai([&](SDK::ACyberneticCharacter* ai) {
				if (ai->IsA(SDK::ASWATCharacter::StaticClass()) || !ron::is_active(ai))
					return;

				auto movement = ai->CharacterMovement;
				if (!movement || movement->MovementMode == SDK::EMovementMode::MOVE_None)
					return;

				movement->DisableMovement();
				track(m_frozen, ai);
			});
		}

		void on_disable() override
		{
			for (const auto& entry : m_frozen)
			{
				auto ai = entry.get();
				if (ai && ron::is_active(ai) && ai->CharacterMovement && ai->CharacterMovement->MovementMode == SDK::EMovementMode::MOVE_None)
					ai->CharacterMovement->SetMovementMode(SDK::EMovementMode::MOVE_Walking, 0);
			}
			m_frozen.clear();
		}
	};

	static freeze_ai _freeze_ai("freeze_ai", "Freeze AI", "Suspects and civilians stop moving (host only)");

	// Suspects pick no target: the player's own no-target cheat, toggled the way god mode is, and the same flag on the
	// squad. The host decides who the AI may target, so there the flags alone are enough even if the toggle did nothing.
	class ai_hold_fire : public looped_command
	{
		using looped_command::looped_command;

		// The pawn this feature last toggled, so a respawned pawn is toggled again but the same one is not toggled twice.
		weak_object<SDK::APlayerCharacter> m_toggled;
		std::vector<weak_object<SDK::ACyberneticCharacter>> m_squad;

		static void toggle(SDK::APlayerCharacter* c, SDK::APlayerController* controller)
		{
			if (controller->HasAuthority())
				c->Server_ToggleNoTarget();
			else
				c->ToggleNoTarget();
		}

		void on_enable() override
		{
			m_toggled = {};
			on_tick();
		}

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			if (!c || !controller)
				return;

			if (!c->bNoTarget && m_toggled.get() != c)
			{
				toggle(c, controller);
				m_toggled = c;
			}

			if (!controller->HasAuthority())
				return;

			c->bNoTarget = true;
			ron::for_each_ai([&](SDK::ACyberneticCharacter* ai) {
				if (ai->IsA(SDK::ASWATCharacter::StaticClass()) && !ai->bNoTarget)
				{
					ai->bNoTarget = true;
					track(m_squad, ai);
				}
			});
		}

		void on_disable() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			m_toggled = {};

			if (c && controller && c->bNoTarget)
			{
				toggle(c, controller);
				if (c->bNoTarget && controller->HasAuthority())
					c->bNoTarget = false;
			}

			for (const auto& member : m_squad)
			{
				if (auto ai = member.get())
					ai->bNoTarget = false;
			}
			m_squad.clear();
		}
	};

	static ai_hold_fire _ai_ignore("ai_ignore", "AI Hold Fire", "Suspects target neither you nor your squad");

	static debug_toggle _objective_markers("objective_markers", "Objective Markers", "The game's markers on every objective",
	    [](debug_subsystem* s) -> bool { return s->bShowObjectiveMarkers; },
	    [](debug_subsystem* s, bool on) { s->bShowObjectiveMarkers = on; },
	    [](debug_subsystem* s) { s->ToggleObjectiveMarkers(); });

	static debug_toggle _evidence_markers("evidence_markers", "Evidence Markers", "The game's markers on every piece of evidence",
	    [](debug_subsystem* s) -> bool { return s->bShowAllEvidenceActors; },
	    [](debug_subsystem* s, bool on) { s->bShowAllEvidenceActors = on; },
	    [](debug_subsystem* s) { s->ToggleAllEvidenceActorMarkers(); });

	static debug_toggle _infinite_swat_items("infinite_swat_items", "Infinite Squad Items", "Your AI squad never runs out of grenades or tools",
	    [](debug_subsystem* s) -> bool { return s->bInfiniteSWATItems; },
	    [](debug_subsystem* s, bool on) { s->bInfiniteSWATItems = on; },
	    [](debug_subsystem* s) { s->ToggleInfiniteSWATItems(); });

	static float_command _time_scale("time_scale", "Time Scale", "Game speed while Slow Motion is on", 0.1f, 1.f, 0.4f);

	class slow_motion : public looped_command
	{
		using looped_command::looped_command;

		// The world's time dilation replicates, so the host slows the lobby down; a client changing it would only
		// desync itself.
		static void set_dilation(float scale)
		{
			auto world = SDK::UWorld::GetWorld();
			if (world && std::abs(SDK::UGameplayStatics::GetGlobalTimeDilation(world) - scale) > 0.001f)
				SDK::UGameplayStatics::SetGlobalTimeDilation(world, scale);
		}

		void on_tick() override
		{
			if (ron::is_host())
				set_dilation(std::clamp(_time_scale.get_state(), _time_scale.get_minimum(), _time_scale.get_maximum()));
		}

		void on_disable() override
		{
			if (ron::is_host())
				set_dilation(1.f);
		}
	};

	static slow_motion _slow_motion("slow_motion", "Slow Motion", "Slows the whole game down (host only)");

	class reset_roster_stress : public command
	{
		using command::command;

		void on_call() override
		{
			// Officers' stress belongs to the Commander roster; outside it the call changes nothing.
			auto controller = ron::controller();
			if (!controller || !SDK::UBpGameplayHelperLib::IsCommanderMode(controller))
			{
				notification::warning(get_label(), "Only works in Commander mode.");
				return;
			}

			controller->SetRosterStressValue(0.f);
			notification::success(get_label(), "Squad stress reset.");
		}
	};

	static reset_roster_stress _reset_roster_stress("reset_roster_stress", "Reset Squad Stress", "Commander mode: brings every officer's stress back to zero");
}
