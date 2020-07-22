#pragma once
#include <string>

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

bool vector_contains(std::vector<std::string> vec, std::string str);
bool is_number(const std::string& s);
bool compare_by_name(const app_entry& a, const app_entry& b);
std::string pad_string_with_spaces(std::string initial, int ending, unsigned int padding_amount);
void print_debug(std::string str);
