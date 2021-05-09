#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <main.h>

nlohmann::json errored_entry(std::string path);
nlohmann::json read_nacp_from_file(std::string path);
bool read_icon_from_file(std::string path, app_entry* current);