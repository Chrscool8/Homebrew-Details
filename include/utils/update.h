#pragma once
#include <string>

struct s_online_info
{
    bool update_available;
    std::string online_version_number;
    std::string online_version_name;
    std::string online_version_description;
};

void init_online_info();
std::string get_online_version_name();
std::string get_online_version_number();
std::string get_online_version_description();
bool get_online_version_available();
void set_online_version_name(std::string name);
void set_online_version_number(std::string number);
void set_online_version_description(std::string desc);
void set_online_version_available(bool available);
bool check_for_updates();