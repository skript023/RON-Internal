
#pragma once

namespace big
{
	struct GPU
	{
		std::string Name;
		std::string AdapterRAM;
		std::string DriverVersion;

		nlohmann::json to_json()
		{
			return *this;
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<GPU>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GPU, Name, AdapterRAM, DriverVersion)
	};
}