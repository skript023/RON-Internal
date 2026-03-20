#pragma once

namespace big
{
	struct OS
	{
		std::string name;
		std::string version;
		std::string kernel;
		std::string architecture;
		std::string endianess;

		nlohmann::json to_json()
		{
			return *this;
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<OS>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(OS, name, version, kernel, architecture, endianess)
	};
}