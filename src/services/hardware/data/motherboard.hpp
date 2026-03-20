#pragma once

namespace big
{
	struct Motherboard
	{
		std::string Manufacturer;
		std::string Name;
		std::string SerialNumber;
		std::string SKU;
		std::string Product;

		nlohmann::json to_json()
		{
			return *this;
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<Motherboard>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Motherboard, Manufacturer, Name, SerialNumber, SKU, Product)
	};
}