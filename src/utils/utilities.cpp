#include <string.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <algorithm>
#include <nlohmann/json.hpp>
#include <pages/main_page.hpp>
#include <string>

std::string json_load_value_string(nlohmann::json json, std::string key)
{
    if (json.contains(key))
        return json[key].get<std::string>();
    else
        return "---";
}

std::string parse_version(std::string version)
{
    if (!version.empty())
    {
        if (version.at(version.length() - 1) == 'd')
            version = version.substr(0, version.length() - 1);
        if (version.at(0) == 'v')
            version = version.substr(1);
    }

    if (!version.empty())
        return version;
    else
        return "0";
}

bool is_number(const std::string& s)
{
    return (s.length() > 0 && strspn(s.c_str(), "-.0123456789") == s.size());
}

bool vector_contains(std::vector<std::string> vec, std::string str)
{
    if (vec.empty())
        return false;
    else
        return (std::find(vec.begin(), vec.end(), str) != vec.end());
}

bool compare_by_name(const app_entry& a, const app_entry& b)
{
    std::string _a = a.name;
    transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
    std::string _b = b.name;
    transform(_b.begin(), _b.end(), _b.begin(), ::tolower);

    if (a.favorite == b.favorite)
    {
        if (_a != _b)
            return (_a.compare(_b) < 0);
        else
            return (a.version.compare(b.version) > 0);
    }
    else
    {
        if (a.favorite)
            return true;
        else
            return false;
    }
}

std::string pad_string_with_spaces(std::string initial, int ending, unsigned int padding_amount)
{
    std::string str = "(" + std::to_string(ending) + ")";
    while ((str.length()) < padding_amount)
        str = " " + str;
    str = initial + str;
    return str;
}

void print_debug(std::string str)
{
    if (get_setting_true(setting_debug))
    {
        str = "[DETAILS] " + str;
        printf(str.c_str());
    }
}

bool replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}