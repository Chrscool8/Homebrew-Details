#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct app_entry
{
    std::string name;
    std::string file_name;
    std::string full_path;
    std::string author;
    std::string version;
    std::size_t size;
    std::size_t icon_size;
    uint8_t* icon;
    bool from_appstore;
    std::string url;
    std::string category;
    std::string license;
    std::string description;
    std::string summary;
    std::string changelog;
    std::string manifest_path;
    bool favorite;
};

extern std::vector<std::string> favorites;
extern std::vector<std::string> blacklist;

extern std::string asset_path;
extern std::string config__path;

extern nlohmann::json apps_info_json;
extern nlohmann::json store_info_json;
