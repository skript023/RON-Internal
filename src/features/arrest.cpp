#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
	class arrest : public command
	{
		using command::command;

		void on_call() override
		{
            auto world = SDK::UWorld::GetWorld(); if (!world) return;
            auto level = world->PersistentLevel; if (!level) return;

            if (auto actors = level->Actors; actors.Num() > 0)
            {
                for (size_t i = 0; i < actors.Num(); i++)
                {
                    if (!actors.IsValidIndex(i)) continue;

                    auto actor = actors[i];

                    if (!actor) continue;

                    if (actor->IsA(SDK::AReadyOrNotCharacter::StaticClass()))
                    {
                        if (auto target_pawn = static_cast<SDK::AReadyOrNotCharacter*>(actor))
                            if (target_pawn->DefaultTeam == SDK::ETeamType::TT_CIVILIAN || target_pawn->DefaultTeam == SDK::ETeamType::TT_SUSPECT)
                            {
                                if (!unreal_engine::get_character()) return;

                                target_pawn->ArrestComplete(unreal_engine::get_character(), nullptr);
                                unreal_engine::get_character()->Server_ReportToTOC(target_pawn, false, false);
                            }
                    }
                }
            }
		}
	};

	static arrest _arrest("auto_arrest", "Auto Arrest", "");
}