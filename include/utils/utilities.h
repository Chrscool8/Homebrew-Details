#pragma once
#include <main.h>

#include <borealis.hpp>
#include <nlohmann/json.hpp>
#include <string>

bool vector_contains(std::vector<std::string> vec, std::string str);
bool is_number(const std::string& s);
bool compare_by_name(const app_entry& a, const app_entry& b);
std::string pad_string_with_spaces(std::string initial, int ending, unsigned int padding_amount);
void print_debug(std::string str);
int json_load_value_int(nlohmann::json json, std::string key);
std::string json_load_value_string(nlohmann::json json, std::string key);
std::string parse_version(std::string version);
void string_replace(std::string& str, const std::string& from, const std::string& to);
std::uint32_t get_battery_percent();
std::string get_battery_status();
std::string digits_string(int value, int digitsCount);
std::string get_time();
std::string get_date();
std::string get_cache_path(std::string str);
std::string get_resource_path(std::string str);
std::string get_config_path(std::string str);
std::string to_megabytes(unsigned int size);
std::string to_gigabytes(uint64_t size);
std::string get_free_space();
std::string get_keyboard_input(std::string default_str);
std::vector<std::string> explode(std::string const& s, char delim);
std::string to_lower(std::string str);
void create_directories(std::string path);
bool copy_file(const char* srce_file, const char* dest_file);
std::string symbol_star();
std::string symbol_downarrow();
std::string symbol_rightarrow();
std::string folder_of_file(std::string filename);
std::string immediate_folder_of_file(std::string filename);
brls::AppletFrame* show_framed(brls::View* view);
brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length);
