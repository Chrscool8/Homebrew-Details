#pragma once
#include <utils/base64.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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
    setting_lax_store_compare,
    setting_control_scheme,
    setting_scan_settings_changed,
    setting_previous_num_files,
    setting_exit_to,
    settings_num
};

////////

void file_load_settings();
void file_save_settings();
void set_setting(int setting, std::string value);
std::string get_setting(int setting);
void init_settings();
bool get_setting_true(int setting);
