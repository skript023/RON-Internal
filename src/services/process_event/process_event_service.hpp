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

		void interceptor_impl(SDK::UObject* _this, SDK::UFunction* function, void* parms);
	public:
		static void interceptor(SDK::UObject* _this, SDK::UFunction* function, void* parms)
		{
			get().interceptor_impl(_this, function, parms);
		}
	};
}