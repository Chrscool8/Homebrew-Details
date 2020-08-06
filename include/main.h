#pragma once

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
    bool favorite;
};

extern std::vector<std::string> favorites;
extern std::vector<std::string> blacklist;
extern std::vector<app_entry> local_apps;
extern std::vector<app_entry> store_apps;
extern std::vector<app_entry> store_file_data;
extern std::string asset_path;
