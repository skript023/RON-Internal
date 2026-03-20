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

		int m_state = 0;
		NumberType m_min;
		NumberType m_max;

	public:
		number_command(std::string name, std::string label, std::string description, NumberType min, NumberType max, int def_val = 0) :
			command(name, label, description, 0),
			m_min(min),
			m_max(max),
			m_state(def_val)
		{
		}
		NumberType get_state() { return m_state; };
		void set_state(int state)
		{
			on_change();
			m_state = state;
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