#include "menu_settings.hpp"
#include "file_manager.hpp"
#include "nlohmann/json.hpp"

namespace big
{
    std::filesystem::path menu_settings::get_file_path()
    {
        return g_file_manager.get_project_file("./menu_settings.json").get_path();
    }

    void menu_settings::attempt_save()
    {
        nlohmann::json j = *this;

        if (deep_compare(this->options, j, true))
            this->save();
    }

    bool menu_settings::load()
    {
        this->default_options = *this;

        const auto settings_file = get_file_path();

        std::ifstream file(settings_file);

        if (!file.is_open())
        {
            this->write_default_config();

            file.open(settings_file);
        }

        try
        {
            file >> this->options;
        }
        catch (const std::exception&)
        {
            LOG(INFO) << "Detected corrupt menu_settings, writing default config...";

            this->write_default_config();

            return this->load();
        }

        bool should_save = this->deep_compare(this->options, this->default_options);

        from_json(this->options, *this);

        if (should_save)
        {
            LOG(INFO) << "Updating menu_settings...";
            save();
        }

        return true;
    }
    bool menu_settings::deep_compare(nlohmann::json &current_settings, const nlohmann::json &default_settings, bool compare_value)
    {
        bool should_save = false;

        for (auto& e : default_settings.items())
        {
            const std::string& key = e.key();

            if (current_settings.count(key) == 0 || (compare_value && current_settings[key] != e.value()))
            {
                current_settings[key] = e.value();

                should_save = true;
            }
            else if (current_settings[key].is_object() && e.value().is_object())
            {
                if (deep_compare(current_settings[key], e.value(), compare_value))
                    should_save = true;
            }
            else if (!current_settings[key].is_object() && e.value().is_object())
            {
                current_settings[key] = e.value();

                should_save = true;
            }
            else if (current_settings[key].size() < e.value().size())
            {
                current_settings[key] = e.value();

                should_save = true;
            }
        }

        return should_save;
    }
    bool menu_settings::save()
    {
        std::ofstream file(get_file_path(), std::ios::out | std::ios::trunc);
        nlohmann::json j = *this;
        file << j.dump(4);
        file.close();

        return true;
    }
    bool menu_settings::write_default_config()
    {
        std::ofstream file(get_file_path(), std::ios::out);
        nlohmann::json j = *this;
        file << j.dump(4);
        file.close();

        return true;
    }
}
