#include "commands.hpp"
#include "command.hpp"
#include "bool_command.hpp"
#include "looped_command.hpp"
#include "process_event/process_event.hpp"

namespace big
{
	commands::commands() :
	    state_serializer("commands")
	{
	}

	void commands::add_command_impl(command* command)
	{
		m_commands.insert({command->get_hash(), command});
	}

	void commands::add_bool_command_impl(bool_command* command)
	{
		m_bool_commands.push_back(command);
	}

	void commands::add_looped_command_impl(looped_command* command)
	{
		m_looped_commands.push_back(command);
	}

	void commands::add_process_event_command_impl(process_event* command)
	{
		m_process_event_commands.insert({ command->get_hash(), command });
	}

	void commands::enable_bool_commands_impl()
	{
		for (auto& command : m_bool_commands)
			if (command->get_state())
				command->initialize();
	}

	void commands::run_looped_commands_impl()
	{
		for (auto& command : m_looped_commands)
			if (command->get_state())
				command->tick();
	}

	command* commands::get_command_impl(joaat_t hash)
	{
		if (auto it = m_commands.find(hash); it != m_commands.end())
			return it->second;
		return nullptr;
	}
	
	bool_command* commands::get_bool_command_impl(joaat_t hash)
	{
		for (auto command : m_bool_commands)
			if (command->get_hash() == hash)
				return command;
		return nullptr;
	}

	process_event* commands::get_process_event_impl(joaat_t hash)
	{
		if (auto it = m_process_event_commands.find(hash); it != m_process_event_commands.end())
			return it->second;
		return nullptr;
	}

	void commands::save_state_impl(nlohmann::json& state)
	{
		for (auto& command : m_commands)
		{
			if (!state.contains(command.second->get_name()))
				state[command.second->get_name()] = nlohmann::json::object();

			command.second->save_state(state[command.second->get_name()]);
		}
	}

	void commands::load_state_impl(nlohmann::json& state)
	{
		for (auto& command : m_commands)
		{
			if (state.contains(command.second->get_name()))
				command.second->load_state(state[command.second->get_name()]);
		}
	}

	void commands::apply_external_impl(nlohmann::json& state, const nlohmann::json& changes)
	{
		for (const auto& [name, value] : changes.items())
		{
			const auto hash = joaat(name);
			if (auto toggle = get_bool_command_impl(hash))
			{
				// set_state, not load_state, so the feature switches on or off exactly like a menu click.
				if (value.is_boolean() && toggle->get_state() != value.get<bool>())
					toggle->set_state(value.get<bool>());
			}
			else if (auto command = get_command_impl(hash); command && !value.is_null())
			{
				auto copy = value;
				command->load_state(copy);
			}
		}
	}

	void commands::shutdown_impl()
	{
		for (auto& command : m_looped_commands)
			if (command->get_state())
				command->shutdown();
	}
}