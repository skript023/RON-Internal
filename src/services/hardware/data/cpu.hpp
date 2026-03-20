#pragma once

namespace big
{
	struct CPU
	{
		std::string Manufacturer;
		std::string Name;
		std::string NumberOfCores;
		std::string MaxClockSpeed;
		std::string NumberOfLogicalProcessors;

		nlohmann::json to_json()
		{
			return *this;
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<CPU>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CPU, Manufacturer, Name, NumberOfCores, MaxClockSpeed, NumberOfLogicalProcessors)
	};
}