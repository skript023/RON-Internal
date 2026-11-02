#include "bool_command.hpp"
#include "task_mgr.hpp"
#include "commands.hpp"

namespace big
{
	bool_command::bool_command(std::string name, std::string label, std::string description, bool def_value) :
	    command(name, label, description, 0),
	    m_state(def_value)
	{
		commands::add_bool_command(this);
	}

	void bool_command::on_call()
	{
		set_state(!m_state);
	}

	void bool_command::save_state(nlohmann::json& value)
	{
		value = m_state;
	}

	void bool_command::load_state(nlohmann::json& value)
	{
		m_state = value;
	}

	bool& bool_command::get_state()
	{
		return m_state;
	}

	void bool_command::set_state(bool state)
	{
		if (state != m_state)
		{
			// The menu switches toggles from the render thread; the game is only touched from the game thread.
			auto apply = [this, state] {
				if (state)
					on_enable();
				else
					on_disable();
			};

			async_run(std::move(apply));
		}

		m_state = state;

		mark_dirty();
	}

	void bool_command::initialize()
	{
		on_enable();
	}

	void bool_command::shutdown()
	{
		on_disable();
	}
}