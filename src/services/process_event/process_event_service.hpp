#pragma once
#include "SDK/Engine_classes.hpp"
#include "interception_status.hpp"

namespace big
{
	using interceptor_t = std::function<eExecutionStatus(SDK::UObject* _this, SDK::UFunction* function, void* parms)>;
	class process_event_service
	{
		static process_event_service& get()
		{
			static process_event_service instance;
			return instance;
		}

		explicit process_event_service() = default;
		~process_event_service() noexcept = default;

		bool interceptor_impl(SDK::UObject* _this, SDK::UFunction* function, void* parms);
	public:
		// Runs the trainer's handler for this function, if it has one. False means the handler already ran the
		// game's function or replaced it, so the hook must not run it.
		static bool interceptor(SDK::UObject* _this, SDK::UFunction* function, void* parms)
		{
			return get().interceptor_impl(_this, function, parms);
		}
	};
}