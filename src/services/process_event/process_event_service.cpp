#include "process_event_service.hpp"

#include "hooking.hpp"
#include "script_mgr.hpp"

#include "process_event.hpp"
#include <commands/commands.hpp>

#include "features/game_tick.hpp"
#include "utility/unreal_engine_utility.hpp"
#include "notification/notification_service.hpp"

#include <shared_mutex>

namespace big
{
	struct function_info
	{
		std::int32_t index;
		joaat_t hash;
		// A per-frame Blueprint event, matched by its own name so a class's override counts as well.
		bool frame_event;
	};

	// Every ProcessEvent in the game comes through here, so each function's names are built once. The object index is
	// kept too: an unloaded Blueprint can free a function whose address is later reused.
	static function_info info_of(SDK::UFunction* function)
	{
		static std::shared_mutex mutex;
		static std::unordered_map<SDK::UFunction*, function_info> infos;

		{
			std::shared_lock lock(mutex);
			if (auto it = infos.find(function); it != infos.end() && it->second.index == function->Index)
				return it->second;
		}

		const auto name = function->GetName();
		const function_info info{ function->Index, joaat(function->GetFullName()),
			name == "ReceiveTick" || name == "BlueprintUpdateAnimation" || name == "Tick" };
		std::unique_lock lock(mutex);
		infos.insert_or_assign(function, info);
		return info;
	}

	bool process_event_service::interceptor_impl(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		if (g_settings.debug.logging_process_event)
			LOG(INFO) << function->GetFullName();

		const auto info = info_of(function);
		if (info.frame_event)
			game_tick::on_frame_event();

		if (auto command = commands::get_process_event(info.hash); command)
			return command->call(_this, function, parms) != eExecutionStatus::EXECUTION_INTERCEPTED;

		return true;
	}
}