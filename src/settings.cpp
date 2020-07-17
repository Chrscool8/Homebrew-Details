#include "settings.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "base64.h"

namespace fs = std::filesystem;

std::string settings[settings_num + 1];

std::string config_path = "sdmc:/config/homebrew_details/";

void file_load_settings()
{
    std::ifstream inputFile(config_path + "config.txt");
    if (inputFile)
    {
        int index = 0;
        while (inputFile)
        {
            char line[513];
            inputFile.getline(line, 512);
            settings[index] = line;
            printf(line);
            printf("\n");
            index += 1;
        }
        inputFile.close();
    }
    else
        printf("Can't find loadfile.\n");
}

void file_save_settings()
{
    if (!fs::exists("sdmc:/config/"))
        fs::create_directory("sdmc:/config/");
    if (!fs::exists("sdmc:/config/homebrew_details/"))
        fs::create_directory("sdmc:/config/homebrew_details/");

    remove((config_path + "config.txt").c_str());
    std::ofstream outputFile(config_path + "config.txt");
    if (outputFile)
    {
        int index = 0;
        while (index < settings_num)
        {
            outputFile << settings[index].c_str() << std::endl;
            index += 1;
        }
        outputFile.close();
    }
    else
        printf("Can't open savefile.\n");
}

void set_setting(int setting, std::string value)
{
    settings[setting] = base64_encode(value);
    printf(("Set " + std::to_string(setting) + " to " + value).c_str());
    file_save_settings();
}

std::string get_setting(int setting)
{
    return base64_decode(settings[setting]);
}

void init_settings()
{
    if (get_setting(setting_search_subfolders) == "")
    {
        set_setting(setting_search_subfolders, "true");
    }

    if (get_setting(setting_search_root) == "")
    {
        set_setting(setting_search_root, "false");
    }

    if (get_setting(setting_scan_full_card) == "")
    {
        set_setting(setting_scan_full_card, "false");
    }

    if (get_setting(setting_autoscan) == "")
    {
        set_setting(setting_autoscan, "false");
    }
}