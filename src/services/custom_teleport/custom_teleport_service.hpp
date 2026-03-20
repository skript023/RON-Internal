#pragma once

namespace big
{
	struct teleport_location
	{
		std::string name;
		float x, y, z;
		float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(teleport_location, name, x, y, z, yaw, pitch, roll)

	class custom_teleport_service
	{
	public:
		std::map<std::string, std::vector<teleport_location>> all_saved_locations;
		bool fetch_saved_locations();
		bool save_new_location(const std::string& category, teleport_location t);
		bool delete_saved_location(const std::string& category, const std::string& location_name);
		teleport_location* get_saved_location_by_name(std::string);
		std::vector<teleport_location> saved_locations_filtered_list(std::string filter = "");

	private:
		std::filesystem::path get_telelocations_file_path();
	};

	inline custom_teleport_service g_custom_teleport_service{};
}