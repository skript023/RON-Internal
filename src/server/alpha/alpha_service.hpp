#pragma once
#include "data/status.hpp"
#include "data/hardware.hpp"
#include "server/socket_client.hpp"
#include "server/interfaces/message.hpp"

#include "hardware/hardware_service.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	class alpha_service
	{
	public:
		explicit alpha_service(): m_status(eIntegrationStatus::UNKNOWN)
		{
			m_alpha_gateway = std::make_shared<socket_client>("server.imost.id/ws/alpha");
			m_alpha_gateway->on_message_received([] (std::string const& message) {
				g_notification_service.info("Server Notification", message);
			});
		}

		~alpha_service() noexcept
		{
			m_alpha_gateway.reset();
		}

		void find()
		{
			m_event.event = "findAllAlpha";
			
			m_alpha_gateway->send_message(m_event.to_json_string());
			m_alpha_gateway->on_message_received([](const std::string& message) {
				if (message.empty())
				{
					
				}
				else
				{

				}
			});
		}

		void ping()
		{
			m_event.event = "pingAlpha";
			
			m_alpha_gateway->send_message(m_event.to_json_string(), [this](std::string const& message) {
				if (message.empty())
				{
					LOG(INFO) << "Received an empty message";
				}
				else
				{

				}
			});
		}

		void send_hardware()
		{
			/*Message<Hardware> subs;
			subs.event = "createAlpha";

			auto os = g_hardware_service.get_os_info();
			auto cpu = g_hardware_service.get_cpu_info();
			auto gpu = g_hardware_service.get_gpu_info();
			auto motherboard = g_hardware_service.get_motherboard_info();

			subs.data = { os, cpu.Name, std::format("{}, Memory: {}", gpu.Name, gpu.AdapterRAM), std::format("{} {}", motherboard.Manufacturer, motherboard.Product)};

			m_alpha_gateway->send_message(subs.to_json_string(), [this](std::string const& message) {
				if (message.empty())
				{
					LOG(INFO) << "Received an empty message";
				}
				else
				{
					LOG(INFO) << "Received: " << message;
					m_status = eIntegrationStatus::CONNECTED;
				}
			});*/
		}

		bool auto_reconnect()
		{
			if (!m_alpha_gateway->is_connected())
			{
				m_status = eIntegrationStatus::DISCONNECTED;

				g_notification_service.info("Ellohim Server", "Reconnecting to Ellohim Server.");

				if (auto reconnect = m_alpha_gateway->reconnect(); !reconnect)
				{
					g_notification_service.info("Ellohim Server", "Reconnect Failed.");

					return false;
				}

				m_status = eIntegrationStatus::CONNECTED;

				return true;
			}

			return false;
		}

		bool is_integrated() const
		{
			return m_status == eIntegrationStatus::CONNECTED;
		}
	private:
		std::shared_ptr<socket_client> m_alpha_gateway;
		Gateway m_event;
		eIntegrationStatus m_status;
	};
}
