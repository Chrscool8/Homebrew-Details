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

nlohmann::json settings_json;

void read_settings()
{
    if (std::filesystem::exists(get_settings_path()))
    {
        std::ifstream file(get_settings_path());
        settings_json = nlohmann::json::parse(file);
    }
    else
        print_debug("Settings json not found.");
}

void save_settings()
{
    create_directories(get_config_path());
    std::ofstream o(get_settings_path());
    o << settings_json << std::endl;
    print_debug("Saving settings");
}

void settings_set_value(std::string category, std::string key, std::string value)
{
    nlohmann::json j_sub;
    if (settings_json.contains(category))
        j_sub = settings_json[category];

    j_sub[key]              = value;
    settings_json[category] = j_sub;
    print_debug("Set " + key + " to " + value);
    save_settings();
}

std::string settings_get_value(std::string category, std::string key)
{
    if (settings_json.contains(category))
    {
        nlohmann::json j_sub = settings_json[category];

        if (j_sub.contains(key))
        {
            if (key != "debug")
                printf(std::string("[DETAILS] Reading setting " + category + " " + key + "\n").c_str());
            return (j_sub[key]);
        }
        else
        {
            printf(std::string("[DETAILS] Heads up! Setting: " + category + ", >" + key + "< not found.\n").c_str());
            return "---";
        }
    }
    else
    {
        printf(std::string("[DETAILS] Heads up! Setting: >" + category + "<, " + key + " not found.\n").c_str());
        return "---";
    }
}

bool settings_get_value_true(std::string category, std::string key)
{
    return (settings_get_value(category, key) == "true");
}

void initialize_setting(std::string category, std::string setting, std::string initial)
{
    if (settings_get_value(category, setting) == "---")
    {
        settings_set_value(category, setting, initial);
    }
}

void init_settings()
{
    initialize_setting("scan", "subfolders", "true");
    initialize_setting("scan", "root", "true");
    initialize_setting("scan", "full card", "false");
    initialize_setting("scan", "autoscan", "false");

    //if (settings_get_value_true("meta", "debug"))
    //    settings_set_value("meta", "local version", std::string("") + APP_VERSION + "d");
    //else
    //    settings_set_value("meta", "local version", APP_VERSION);

    initialize_setting("preferences", "control scheme", "0");
    initialize_setting("old", "lax store compare", "false");
    initialize_setting("scan", "settings changed", "true");
    initialize_setting("history", "previous number of files", "1");
    initialize_setting("meta", "exit to", "sdmc:/hbmenu.nro");
    initialize_setting("sort", "main", "name");
    initialize_setting("sort", "secondary", "version");
    initialize_setting("sort", "direction", "ascending");
    initialize_setting("sort", "grouping", "");

    if (settings_get_value("history", "last seen version") == "---" || (APP_VERSION != settings_get_value("history", "last seen version")))
    {
        settings_set_value("history", "last seen version", APP_VERSION);
        print_debug("DIFFERING VERSION!!");
        settings_set_value("internal", "invalidate cache", "true");
    }
}
