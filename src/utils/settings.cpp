#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

//std::string settings[settings_num + 1];

std::string settings_path = get_config_path("settings.json");
nlohmann::json settings_json;

void read_settings()
{
    if (std::filesystem::exists(settings_path))
    {
        std::ifstream i(settings_path);
        settings_json.clear();
        i >> settings_json;
    }
}

void save_settings()
{
    create_directories(get_config_path(""));
    std::ofstream o(settings_path);
    o << settings_json << std::endl;
}

void settings_set_value(std::string key, std::string value)
{
    settings_json[key] = value;
    print_debug("Set " + key + " to " + value);
    save_settings();
}

std::string settings_get_value(std::string key)
{
    if (settings_json.contains(key))
        return (settings_json[key]);
    else
        return "---";
}

bool settings_get_value_true(std::string key)
{
    return (settings_get_value(key) == "true");
}

void initialize_setting(std::string setting, std::string initial)
{
    if (settings_get_value(setting) == "---")
    {
        settings_set_value(setting, initial);
    }
}

void init_settings()
{
    initialize_setting(setting_search_subfolders, "true");
    initialize_setting(setting_search_root, "false");
    initialize_setting(setting_scan_full_card, "false");
    initialize_setting(setting_autoscan, "false");

    if (settings_get_value_true(setting_debug))
        settings_set_value(setting_local_version, std::string(APP_VERSION) + "d");
    else
        settings_set_value(setting_local_version, APP_VERSION);

    initialize_setting(setting_control_scheme, "0");
    initialize_setting(setting_lax_store_compare, "false");
    initialize_setting(setting_scan_settings_changed, "true");
    initialize_setting(setting_previous_num_files, "1");
    initialize_setting(setting_exit_to, "sdmc:/hbmenu.nro");
    initialize_setting(setting_sort_type, "name");
    initialize_setting(setting_sort_type_2, "version");
    initialize_setting(setting_sort_direction, "ascending");
    initialize_setting(setting_sort_group, "");

    if (settings_get_value(setting_last_seen_version) == "---" || (APP_VERSION != settings_get_value(setting_last_seen_version)))
    {
        settings_set_value(setting_last_seen_version, std::string(APP_VERSION));
        print_debug("DIFFERING VERSION!!");
        settings_set_value(setting_invalidate_cache, "true");
    }
}
