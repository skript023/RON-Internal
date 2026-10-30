#include "commands/looped_command.hpp"
#include "features/mission.hpp"
#include "utility/ron.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// A suspect that hears an officer yell, any officer's and not only yours, gives up there and then. The game itself
	// decides who hears it, so this is still a yell in range and not Surrender Suspects, which needs none; the flag pair
	// the game keeps for that is read instead of the yell itself, so nothing has to be hooked.
	class surrender_on_yell : public looped_command
	{
		using looped_command::looped_command;

		// How long after hearing the yell a suspect still gives up over it. Long enough that the flag is not missed
		// between this tick and the last, short enough that an old yell does not count.
		static constexpr float window = 1.f;

		void on_tick() override
		{
			if (!ron::is_host())
				return;

			ron::for_each_ai([](SDK::ACyberneticCharacter* ai) {
				if (ai->IsA(SDK::ASuspectCharacter::StaticClass()) && ai->bHeardYellFromOfficer && ai->TimeSinceHeardOfficerYell < window)
					mission::surrender(ai);
			});
		}
	};

	static surrender_on_yell _surrender_on_yell("surrender_on_yell", "Surrender on Yell", "Suspects give up the moment they hear an officer yell (host only)");
}
