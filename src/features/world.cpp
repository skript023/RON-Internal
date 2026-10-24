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

	static debug_toggle _freeze_ai("freeze_ai", "Freeze AI", "AI stop moving",
	    [](debug_subsystem* s) -> bool { return s->bDisableAIMovement; },
	    [](debug_subsystem* s, bool on) { s->bDisableAIMovement = on; },
	    [](debug_subsystem* s) { s->ToggleAIMovement(); });

	static debug_toggle _ai_ignore("ai_ignore", "AI Hold Fire", "AI stop picking targets",
	    [](debug_subsystem* s) -> bool { return s->bDisableAITargeting; },
	    [](debug_subsystem* s, bool on) { s->bDisableAITargeting = on; },
	    [](debug_subsystem* s) { s->ToggleAITargeting(); });

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

	static float_command _damage_scale("damage_scale", "Damage Scale", "Weapon damage while Damage Multiplier is on", 0.1f, 10.f, 2.f);

	class damage_multiplier : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto subsystem = ron::debug_subsystem();
			if (!subsystem)
				return;

			const float scale = _damage_scale.get_state();
			if (subsystem->GlobalDamageMultiplier_Weapons != scale)
			{
				subsystem->SetGlobalDamageMultiplier_Weapons(scale);
				subsystem->GlobalDamageMultiplier_Weapons = scale;
			}
			if (!subsystem->bApplyGlobalDamageMultiplier_Weapons)
			{
				subsystem->ToggleGlobalDamageMultiplier_Weapons();
				subsystem->bApplyGlobalDamageMultiplier_Weapons = 1;
			}
		}

		void on_disable() override
		{
			auto subsystem = ron::debug_subsystem();
			if (!subsystem || !subsystem->bApplyGlobalDamageMultiplier_Weapons)
				return;

			subsystem->ToggleGlobalDamageMultiplier_Weapons();
			subsystem->bApplyGlobalDamageMultiplier_Weapons = 0;
		}
	};

	static damage_multiplier _damage_multiplier("damage_multiplier", "Damage Multiplier", "Scales the damage every weapon deals");

	class reset_roster_stress : public command
	{
		using command::command;

		void on_call() override
		{
			if (auto controller = ron::controller())
			{
				controller->SetRosterStressValue(0.f);
				notification::success(get_label(), "Squad stress reset.");
			}
		}
	};

	static reset_roster_stress _reset_roster_stress("reset_roster_stress", "Reset Squad Stress", "Commander mode: brings every officer's stress back to zero");
}
