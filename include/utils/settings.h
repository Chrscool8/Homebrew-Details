#pragma once
#include <utils/base64.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern nlohmann::json settings_json;

const std::string setting_search_subfolders     = "search subfolders";
const std::string setting_search_root           = "search root";
const std::string setting_scan_full_card        = "search full card";
const std::string setting_autoscan              = "autoscan";
const std::string setting_nro_path              = "nro path";
const std::string setting_debug                 = "debug mode";
const std::string setting_local_version         = "local version";
const std::string setting_lax_store_compare     = "lax store compare";
const std::string setting_control_scheme        = "control scheme";
const std::string setting_scan_settings_changed = "scan settings changed";
const std::string setting_previous_num_files    = "previous number of files";
const std::string setting_exit_to               = "exit to";
const std::string setting_sort_type             = "sort main";
const std::string setting_sort_type_2           = "sort secondary";
const std::string setting_sort_direction        = "sort direction";
const std::string setting_sort_group            = "sort grouping";
const std::string setting_last_seen_version     = "last seen version";
const std::string setting_invalidate_cache      = "invalidate cache";

////////

//void file_load_settings();
//void file_save_settings();
//void set_setting(int setting, std::string value);
//std::string get_setting(int setting);
void init_settings();
//bool get_setting_true(int setting);

void read_settings();
void save_settings();
void settings_set_value(std::string key, std::string value);
std::string settings_get_value(std::string key);
bool settings_get_value_true(std::string key);