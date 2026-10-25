#include "process_event_service.hpp"

#include "hooking.hpp"
#include "script_mgr.hpp"

#include "process_event.hpp"
#include <commands/commands.hpp>

#include "utility/unreal_engine_utility.hpp"
#include "notification/notification_service.hpp"

#include <shared_mutex>

namespace big
{
	// Every ProcessEvent in the game comes through here, so each function's name is built and hashed once. The
	// object index is kept too: an unloaded Blueprint can free a function whose address is later reused.
	static joaat_t function_hash(SDK::UFunction* function)
	{
		struct cached
		{
			std::int32_t index;
			joaat_t hash;
		};

		static std::shared_mutex mutex;
		static std::unordered_map<SDK::UFunction*, cached> hashes;

		{
			std::shared_lock lock(mutex);
			if (auto it = hashes.find(function); it != hashes.end() && it->second.index == function->Index)
				return it->second.hash;
		}

		const auto hash = joaat(function->GetFullName());
		std::unique_lock lock(mutex);
		hashes.insert_or_assign(function, cached{ function->Index, hash });
		return hash;
	}

	bool process_event_service::interceptor_impl(SDK::UObject* _this, SDK::UFunction* function, void* parms)
	{
		if (g_settings.debug.logging_process_event)
			LOG(INFO) << function->GetFullName();

		auto func = function_hash(function);

		if (auto command = commands::get_process_event(func); command)
			return command->call(_this, function, parms) != eExecutionStatus::EXECUTION_INTERCEPTED;

		return true;
	}
}