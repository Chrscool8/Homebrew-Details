#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "base64.h"

namespace fs = std::filesystem;

enum setting
{
    setting_search_subfolders,
    setting_search_root,
    setting_scan_full_card,
    setting_autoscan,
    setting_nro_path,
    setting_debug,
    setting_local_version,
    settings_num
};

////////

void file_load_settings();
void file_save_settings();
void set_setting(int setting, std::string value);
std::string get_setting(int setting);
void init_settings();
bool get_setting_true(int setting);
