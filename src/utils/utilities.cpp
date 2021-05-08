#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <sys/statvfs.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <algorithm>
#include <borealis.hpp>
#include <nlohmann/json.hpp>
#include <pages/main_page.hpp>
#include <sstream>
#include <string>

#include "switch/services/psm.h"

void export_resource(std::string folder, std::string src)
{
    folder += "/";
    create_directories(get_config_path() + folder);

    if (!fs::exists(get_config_path() + folder + src))
    {
        std::ifstream srcfile(("romfs:/" + src).c_str(), std::ios::binary);
        std::ofstream dstfile(get_config_path() + folder + src, std::ios::binary);

        dstfile << srcfile.rdbuf();
    }
}

void copy_resources()
{
    export_resource("assets", "arrows.png");
    export_resource("assets", "arrows_small.png");
    export_resource("assets", "download.png");
    export_resource("assets", "icon.png");
    export_resource("assets", "warning.png");
    export_resource("assets", "warning_arrows.png");
    export_resource("assets", "style_list.png");
    export_resource("assets", "style_flow.png");
    export_resource("assets", "style_beta.png");
    export_resource("assets", "unknown.png");
    export_resource("assets", "credits.png");

    //export_resource("forwarder", "HomebrewDetailsForwarder_v2.nsp");
}

std::string months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

int json_load_value_int(nlohmann::json json, std::string key)
{
    if (json.contains(key))
        return json[key].get<int>();
    else
        return -1;
}

std::string json_load_value_string(nlohmann::json json, std::string key)
{
    if (json.contains(key))
    {
        if (json[key].is_string())
            return json[key].get<std::string>();
        else
            return std::to_string(json[key].get<int>());
    }
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

std::string to_lower(std::string str)
{
    std::string _str = str;
    transform(_str.begin(), _str.end(), _str.begin(), ::tolower);
    return _str;
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

bool compare_json_by_name(nlohmann::json a, nlohmann::json b)
{
    if (!(a.contains("name") && b.contains("name")))
        return 0;

    std::string _a = json_load_value_string(a, "name");
    transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
    std::string _b = json_load_value_string(b, "name");
    transform(_b.begin(), _b.end(), _b.begin(), ::tolower);

    if (_a != _b)
        return (_a.compare(_b) < 0);
    else
    {
        if (!(a.contains("version") && b.contains("version")))
            return 0;
        else
            return (json_load_value_string(a, "version")).compare(json_load_value_string(b, "version")) < 0;
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
    if (settings_get_value_true("meta", "debug"))
    {
        str = "[DETAILS] " + str + "\n";
        printf(str.c_str());
    }
}

void string_replace(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return;
    if (str.size() < from.size())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::uint32_t get_battery_percent()
{
    std::uint32_t batteryCharge = 0;
    psmGetBatteryChargePercentage(&batteryCharge);
    return batteryCharge;
}

std::string get_battery_status()
{
    ChargerType chargerType;
    // Not Charging, Charging Via Power, Charging via USB
    std::string chargerTypes[3] = { std::string(""), std::string(" Charging"), std::string(" via USB") };
    psmGetChargerType(&chargerType);
    // Error by Default
    std::string chargeStatus = "";
    if ((int)chargerType >= 0 && (int)chargerType < 3)
        chargeStatus = chargerTypes[chargerType];
    return chargeStatus;
}

std::string digits_string(int value, int numDigits)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(numDigits) << value;
    return oss.str();
}

std::string bool_string(bool boo)
{
    if (boo)
        return "true";
    else
        return "false";
}

std::string get_time()
{
    auto t  = std::time(nullptr);
    auto tm = *std::localtime(&t);
    return (digits_string(tm.tm_hour, 2) + ":" + digits_string(tm.tm_min, 2) + ":" + digits_string(tm.tm_sec, 2));
}

std::string get_date()
{
    auto t  = std::time(nullptr);
    auto tm = *std::localtime(&t);
    return (months[tm.tm_mon] + " " + digits_string(tm.tm_mday, 2) + ", " + std::to_string(1900 + tm.tm_year));
}

std::string get_config_path()
{
    return "sdmc:/config/homebrew_details_next/";
}

std::string get_cache_path()
{
    return get_config_path() + "cache/";
}

std::string get_resource_path()
{
    return get_config_path() + "assets/";
}

std::string get_apps_cache_file()
{
    return get_config_path() + "apps_info.json";
}

std::string get_settings_path()
{
    return get_config_path() + "settings.json";
}

std::string get_notes_path()
{
    return get_config_path() + "notes.json";
}

std::string to_megabytes(unsigned int size)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << (size / 1024. / 1024.);
    std::string str = stream.str();
    return str;
}

std::string to_gigabytes(uint64_t size)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << (size / 1024. / 1024. / 1024.);
    std::string str = stream.str();
    return str;
}

std::string get_free_space()
{
    struct statvfs st;
    if (::statvfs("sdmc:/", &st) != 0)
    {
        return "-1";
    }
    else
    {
        uint64_t freeSpace = static_cast<std::uint64_t>(st.f_bsize) * st.f_bfree;
        return (to_gigabytes(freeSpace) + " GB\n");
    }
}

std::string get_keyboard_input(std::string default_str)
{
    SwkbdConfig kbd;
    const unsigned int str_len = 256;
    if (R_SUCCEEDED(swkbdCreate(&kbd, 0)))
    {
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetInitialText(&kbd, default_str.c_str());
        swkbdConfigSetStringLenMax(&kbd, str_len);
        //swkbdConfigSetHeaderText(&kbd, "Header Text");
        //swkbdConfigSetSubText(&kbd, "SubText");
        //swkbdConfigSetGuideText(&kbd, "Guide Text");
        char keyboard_chars[str_len];
        Result res = swkbdShow(&kbd, keyboard_chars, str_len);
        swkbdClose(&kbd);
        if (R_SUCCEEDED(res))
        {
            print_debug(std::string("kbd out: ") + keyboard_chars);
            std::string str = keyboard_chars;
            return str;
        }
        else
        {
            print_debug(std::string("kbd fail."));
        }
    }

    return "";
}

std::vector<std::string> explode(std::string const& s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim);)
    {
        result.push_back(std::move(token));
    }

    return result;
}

void create_directories(std::string path)
{
    if (!fs::exists(path))
    {
        std::vector<std::string> folders = explode(path, '/');
        std::string subpath              = "";
        for (unsigned int i = 0; i < folders.size(); i++)
        {
            subpath += folders.at(i) + "/";
            print_debug(subpath);
            if (!fs::exists(subpath))
                fs::create_directory(subpath);
        }
    }
}

bool copy_file(const char* source_file, const char* target_file)
{
    char buff[BUFSIZ];
    FILE *in, *out;
    size_t n;

    in  = fopen(source_file, "rb");
    out = fopen(target_file, "wb");

    if (in && out)
    {
        while ((n = fread(buff, 1, BUFSIZ, in)) != 0)
        {
            fwrite(buff, 1, n, out);
        }
    }

    if (in)
        fclose(in);

    if (out)
        fclose(out);

    if (!in || !out)
        return false;

    print_debug("done");
    return true;
}

std::string symbol_star()
{
    return "\u2606";
}

std::string symbol_downarrow()
{
    return "\u21E9";
}

std::string symbol_rightarrow()
{
    return "\uE090";
}

std::string symbol_bullet()
{
    return "\u2022";
}

std::string folder_of_file(std::string filename)
{
    size_t found = filename.find_last_of("/\\");
    return (filename.substr(0, found));
}

std::string immediate_folder_of_file(std::string filename)
{
    size_t found = filename.find_last_of("/\\") + 1;
    return (filename.substr(found));
}

brls::AppletFrame* show_framed(brls::View* view)
{
    brls::AppletFrame* frame = new brls::AppletFrame(false, false);
    frame->setContentView(view);
    frame->updateActionHint(brls::Key::B, "");

    brls::Application::pushView(frame);
    return frame;
}

brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length)
{
    brls::ListItem* item = new brls::ListItem(title);

    if (short_info.length() > (unsigned int)clip_length)
    {
        if (long_info.empty())
            long_info = "Full " + title + ":\n\n" + short_info;
        string_replace(short_info, "\\n", " ");
        short_info = short_info.substr(0, clip_length) + "[...]";
    }

    string_replace(long_info, "\\n", "\n");

    item->setValue(short_info);

    if (!long_info.empty())
    {
        item->getClickEvent()->subscribe([long_info](brls::View* view) {
            brls::Dialog* dialog                       = new brls::Dialog(long_info);
            brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                dialog->close();
            };
            dialog->addButton("Dismiss", closeCallback);
            dialog->setCancelable(true);
            dialog->open();
        });
        item->updateActionHint(brls::Key::A, "Show Extended Info");
    }

    if (add_to != NULL)
        add_to->addView(item);

    return item;
}

std::string upper_first_letter(std::string str)
{
    str[0] = toupper(str[0]);
    return str;
}

std::string string_pad_zeroes(std::string number)
{
    while (number.length() < 15)
        number = "0" + number;
    return number;
}

bool string_contains(std::string str, std::string substr)
{
    return (str.find(substr) != std::string::npos);
}