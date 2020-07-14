#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "base64.h"

namespace fs = std::filesystem;

enum setting
{
    setting_recursive_search,
    setting_search_root,
    settings_num
};

////////

void file_load_settings();

void file_save_settings();

void set_setting(int setting, std::string value);
std::string get_setting(int setting);

void init_settings();