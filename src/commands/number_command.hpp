#pragma once
#include "command.hpp"

namespace big
{
	template<typename NumberType>
	class number_command : public command
	{
	protected:
		virtual void on_change() {};
		virtual void on_call() override;
		virtual void save_state(nlohmann::json& value) override
		{
			value = m_state;
		};
		virtual void load_state(nlohmann::json& value) override 
		{
			m_state = value;
		};

		// NumberType, not int: a float command such as the aimbot FOV kept only whole numbers before.
		NumberType m_state{};
		NumberType m_min;
		NumberType m_max;

	public:
		number_command(std::string name, std::string label, std::string description, NumberType min, NumberType max, NumberType def_val = {}) :
			command(name, label, description, 0),
			m_min(min),
			m_max(max),
			m_state(def_val)
		{
		}
		NumberType get_state() { return m_state; };
		void set_state(NumberType state)
		{
			m_state = state;
			on_change();
			mark_dirty();
		};
		NumberType get_minimum()
		{
			return m_min;
		}
		NumberType get_maximum()
		{
			return m_max;
		}
	};
	template<typename NumberType>
	inline void number_command<NumberType>::on_call()
	{}
}