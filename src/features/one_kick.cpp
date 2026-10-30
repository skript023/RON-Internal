#include "commands/looped_command.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// The host rolls each kick against the door's own chance, from 0 to 1, and counts successful kicks down to the one
	// that breaks it; some doors are set to fail every kick. Each door is set to break at the first kick, and put back
	// afterwards.
	class one_kick_doors : public looped_command
	{
		using looped_command::looped_command;

		struct kick_rules
		{
			weak_object<SDK::ADoor> door;
			float chance;
			uint8_t kicks;
			bool always_fails;
		};

		std::vector<kick_rules> m_original;

		void on_tick() override
		{
			auto gs = ron::game_state();
			if (!gs || !ron::is_host())
				return;

			for (auto door : gs->AllDoors)
			{
				if (!unreal_engine::is_valid_actor(door)
				    || (door->DoorKickSuccessChance >= 1.f && door->NumSuccessfulKicksToBreakDown <= 1 && !door->bKickAlwaysFails))
					continue;

				const weak_object<SDK::ADoor> entry(door);
				if (std::ranges::find(m_original, entry, &kick_rules::door) == m_original.end())
				{
					// Doors of earlier missions are gone.
					std::erase_if(m_original, [](const kick_rules& rules) { return !rules.door; });
					m_original.push_back({entry, door->DoorKickSuccessChance, door->NumSuccessfulKicksToBreakDown, static_cast<bool>(door->bKickAlwaysFails)});
				}
				door->DoorKickSuccessChance = 1.f;
				door->NumSuccessfulKicksToBreakDown = (std::min)(door->NumSuccessfulKicksToBreakDown, static_cast<uint8_t>(1));
				door->bKickAlwaysFails = 0;
			}
		}

		void on_disable() override
		{
			for (const auto& rules : m_original)
			{
				if (auto door = rules.door.get())
				{
					door->DoorKickSuccessChance = rules.chance;
					door->NumSuccessfulKicksToBreakDown = rules.kicks;
					door->bKickAlwaysFails = rules.always_fails;
				}
			}
			m_original.clear();
		}
	};

	static one_kick_doors _one_kick_doors("one_kick_doors", "One-Kick Doors", "Every kick breaks a door open at the first try, locked or not (host only)");
}
