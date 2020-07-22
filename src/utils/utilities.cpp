#include <string.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <algorithm>
#include <string>

bool is_number(const std::string& s)
{
    return (s.length() > 0 && strspn(s.c_str(), "-.0123456789") == s.size());
}

bool compare_by_name(const app_entry& a, const app_entry& b)
{
    std::string _a = a.name;
    transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
    std::string _b = b.name;
    transform(_b.begin(), _b.end(), _b.begin(), ::tolower);

    if (_a != _b)
        return (_a.compare(_b) < 0);
    else
        return (a.version.compare(b.version) > 0);
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
        printf(str.c_str());
}