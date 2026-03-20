#pragma once
#include "base_option.hpp"
#include "canvas.hpp"
#include "fiber_pool.hpp"

#include "commands/commands.hpp"
#include "commands/int_command.hpp"
#include "commands/bool_command.hpp"

namespace big
{
	class bool_slider_int_option : public base_option<bool_slider_int_option>
	{
	public:
		explicit bool_slider_int_option(const char* text, const char* description, bool* b00l, int* number, int min, int max, int step = 1, std::size_t precision = 3, bool actionOnHorizontal = true, std::function<void()> action = [] {}) :
			m_bool(b00l),
			m_action_on_horizontal(actionOnHorizontal),
			m_number(number),
			m_min(min),
			m_max(max),
			m_step(step),
			m_precision(precision)
		{
			Base::set_left_text(text);
			if (description)
				Base::set_description(description);
			Base::set_action(std::move(action));
		}
		explicit bool_slider_int_option(uint32_t id_bool, uint32_t id_int, int step = 1, std::size_t precision = 3, bool actionOnHorizontal = true, std::function<void()> action = [] {}) :
			m_bool(nullptr),
			m_number(nullptr),
			m_bool_command(commands::get_command<bool_command>(id_bool)),
			m_int_command(commands::get_command<int_command>(id_int)),
			m_action_on_horizontal(actionOnHorizontal),
			m_step(step),
			m_precision(precision)
		{
			if (!m_bool_command || !m_int_command)
			{
				LOG(FATAL) << "Command " << id_bool << " or " << id_int << " not found.";

				return;
			}

			m_min = *m_int_command->get_minimum();
			m_max = *m_int_command->get_maximum();

			auto& description = m_bool_command->get_description();
			auto& text = m_bool_command->get_label();

			Base::set_left_text(text.c_str());
			if (!description.empty())
				Base::set_description(description.c_str());
			Base::set_action(std::move(action));
		}

		~bool_slider_int_option() noexcept = default;
		bool_slider_int_option(bool_slider_int_option const&) = default;
		bool_slider_int_option& operator=(bool_slider_int_option const&) = default;
		bool_slider_int_option(bool_slider_int_option&&) = default;
		bool_slider_int_option& operator=(bool_slider_int_option&&) = default;

		int get_integer() override
		{
			if (m_int_command)
				return m_int_command->get_state();

			return 0;
		}

		int get_min_integer() override
		{
			return m_min;
		}

		int get_max_integer() override
		{
			return m_max;
		}

		void handle_action(OptionAction action) override
		{
			if (action == OptionAction::EnterPress)
			{
				if (m_bool_command)
					m_bool_command->set_state(!m_bool_command->get_state());
				else
					*m_bool = !*m_bool;
				
			}
			else if (action == OptionAction::LeftPress)
			{
				if (m_int_command)
				{
					if (m_int_command->get_state() - m_step < m_min)
						m_int_command->set_state(m_max);
					else
						m_int_command->set_state(m_int_command->get_state() - m_step);

					if (m_action_on_horizontal && Base::m_action)
						std::invoke(Base::m_action);
				}
				else
				{
					if (*m_number - m_step < m_min)
						*m_number = m_max;
					else
						*m_number -= m_step;

					if (m_action_on_horizontal && Base::m_action)
						std::invoke(Base::m_action);
				}
			}
			else if (action == OptionAction::RightPress)
			{
				if (m_int_command)
				{
					if (m_int_command->get_state() + m_step > m_max)
						m_int_command->set_state(m_min);
					else
						m_int_command->set_state(m_int_command->get_state() + m_step);
				}
				else
				{
					if (*m_number + m_step > m_max)
						*m_number = m_min;
					else
						*m_number += m_step;
				}

				if (m_action_on_horizontal && Base::m_action)
					std::invoke(Base::m_action);
			}

			Base::handle_action(action);
		}

		bool get_flag(OptionFlag flag) override
		{
			if (flag == OptionFlag::BoolSliderInt)
			{
				if (m_bool_command)
					canvas::set_bool_slider_float(m_bool_command->get_state());
				else
					canvas::set_bool_slider_float(*m_bool);

				return true;
			}

			return Base::get_flag(flag);
		}
	private:
		bool* m_bool;
		bool m_action_on_horizontal{};
		int* m_number{};
		int m_min{};
		int m_max{};
		int m_step{};
		std::size_t m_precision{};
		bool_command* m_bool_command{ nullptr };
		int_command* m_int_command{ nullptr };
		using Base = base_option<bool_slider_int_option>;
	};
}
