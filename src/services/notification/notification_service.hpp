#pragma once
#include <astra/host/notification.hpp>

namespace big
{
	// Older call sites keep this name; every card now comes from the Astra queue, drawn by canvas::draw_notifications.
	class notification_service final
	{
	public:
		void push(std::string title, std::string message) { notification::info(std::move(title), std::move(message)); }
		void push_warning(std::string title, std::string message) { notification::warning(std::move(title), std::move(message)); }
		void push_error(std::string title, std::string message) { notification::error(std::move(title), std::move(message)); }

		void success(std::string title, std::string message) { notification::success(std::move(title), std::move(message)); }
		void info(std::string title, std::string message) { notification::info(std::move(title), std::move(message)); }
		void warning(std::string title, std::string message) { notification::warning(std::move(title), std::move(message)); }
		void error(std::string title, std::string message) { notification::error(std::move(title), std::move(message)); }

		void protection(std::string message) { notification::protection(std::move(message)); }
		void report(std::string message) { notification::report(std::move(message)); }
	};

	inline notification_service g_notification_service{};
}
