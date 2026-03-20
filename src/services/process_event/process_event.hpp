#pragma once
#include "utility/joaat.hpp"
#include <nlohmann/json.hpp>

#include "SDK/Engine_classes.hpp"
#include "interception_status.hpp"


namespace big
{
	class process_event
	{
	private:
		std::string m_name;
		joaat_t m_hash;
	protected:
		virtual eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) = 0;
		void mark_dirty();

	public:
		process_event(std::string name);
		eExecutionStatus call(SDK::UObject* _this, SDK::UFunction* function, void* parms);
		eExecutionStatus intercept(SDK::UObject* _this, SDK::UFunction* function, void* parms);

		virtual void save_state(nlohmann::json& value) {};
		virtual void load_state(nlohmann::json& value) {};

		const std::string& get_name()
		{
			return m_name;
		}

		joaat_t get_hash()
		{
			return m_hash;
		}
	};
}