#include <main.h>
#include <utils/base64.h>
#include <utils/notes.h>
#include <utils/utilities.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

nlohmann::json notes_json;

void read_notes()
{
    if (std::filesystem::exists(get_notes_path()))
    {
        std::ifstream i(get_notes_path());
        notes_json.clear();
        i >> notes_json;
    }
}

void save_notes()
{
    std::ofstream o(get_notes_path());
    o << notes_json << std::endl;
}

void notes_set_value(std::string key, std::string value)
{
    notes_json[key] = base64_encode(value);
    save_notes();
}

std::string notes_get_value(std::string key)
{
    if (notes_json.contains(key))
        return base64_decode(notes_json[key]);
    else
        return "";
}