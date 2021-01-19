#pragma once
#include <utils/base64.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern nlohmann::json settings_json;

////////

void init_settings();
void read_settings();
void save_settings();
void settings_set_value(std::string category, std::string key, std::string value);
std::string settings_get_value(std::string category, std::string key);
bool settings_get_value_true(std::string category, std::string key);