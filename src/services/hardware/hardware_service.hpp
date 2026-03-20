#pragma once

#include <subprocess.hpp>
#include "thread_pool.hpp"

#include "data/cpu.hpp"
#include "data/gpu.hpp"
#include "data/motherboard.hpp"

namespace big
{
	class hardware_service
	{
		std::string os_info;
		CPU cpu_info;
		GPU gpu_info;
		Motherboard motherboard_info;
	public:
		explicit hardware_service() = default;
		~hardware_service() noexcept = default;

		hardware_service(hardware_service const&) = delete;
		hardware_service operator=(hardware_service const&) = delete;
		hardware_service(hardware_service&&) = delete;
		hardware_service operator=(hardware_service&&) = delete;

		void init();
		void destroy();

		std::string get_os_info() const
		{
			return os_info;
		}

		CPU get_cpu_info() const
		{
			return cpu_info;
		}

		GPU get_gpu_info() const
		{
			return gpu_info;
		}

		Motherboard get_motherboard_info() const
		{
			return motherboard_info;
		}

		bool load_os_info();
		bool load_cpu_info();
		bool load_gpu_info();
		bool load_motherboard_info();

	private:
		void load_hardware_info()
		{
			load_os_info();
			load_cpu_info();
			load_gpu_info();
			load_motherboard_info();
		}

		std::string trim(const std::string& str)
		{
			size_t first = str.find_first_not_of(" \r\n");
			if (first == std::string::npos)
			{
				return "";
			}
			size_t last = str.find_last_not_of(" \r\n");
			return str.substr(first, last - first + 1);
		}

		std::string BSTRToString(BSTR bstr);
	};
}