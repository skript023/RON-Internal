#include "input_service.hpp"

namespace big
{
	void input_service::show(std::string title)
	{
		strcpy(m_title, title.c_str());
		open();
	}
	void input_service::show(std::string const& title, input_callback_t cb)
	{
		strcpy(m_title, title.c_str());
		open();

		callback(cb);
	}
	void input_service::hide()
	{
		strcpy(m_title, "Input Here");
		memset(m_input, 0, _ARRAYSIZE(m_input));

		// Draw Mouse is the player's saved setting; the box only borrows it while it is open.
		if (g_settings.window.input)
			g_settings.window.mouse_active = m_mouse_before;
		g_settings.window.input = false;
	}
	void input_service::open()
	{
		// Typing reaches the box only while the menu uses the mouse.
		if (!g_settings.window.input)
			m_mouse_before = g_settings.window.mouse_active;
		g_settings.window.input = true;
		g_settings.window.mouse_active = true;
	}
	void input_service::callback(input_callback_t callback)
	{
		if (callback)
		{
			std::lock_guard lock(m_mutex);
			m_callback.push(std::move(callback));
		}
	}
	void input_service::execute()
	{
		std::unique_lock lock(m_mutex);
		if (!m_callback.empty())
		{
			input_callback_t job = std::move(m_callback.top());
			m_callback.pop();
			lock.unlock();

			std::invoke(std::move(job), std::string(m_input));
			this->hide();
		}
	}
}