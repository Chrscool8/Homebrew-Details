#include <dirent.h>
#include <main.h>
#include <utils/nacp_utils.h>
#include <utils/scanning.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <atomic>

namespace fs = std::filesystem;

std::atomic<int> file_count = 0;
std::string last_file_name  = "";

scanprogress scanprog;

void read_store_apps()
{
    store_file_data.clear();
    store_apps.clear();

    std::string path = "/switch/appstore/.get/packages/";
    if (fs::exists(path))
    {
        for (const auto& entry : fs::directory_iterator(path))
        {
            std::string folder = entry.path();
            if (fs::is_directory(folder))
            {
                print_debug(("folder: " + folder + "\n").c_str());
                app_entry current;
                current.from_appstore = false;

                std::string info_file = folder + "/info.json";
                if (fs::exists(info_file))
                {
                    print_debug(("info_file: " + info_file + "\n").c_str());

                    std::ifstream i(info_file);
                    nlohmann::json info_json;
                    i >> info_json;

                    current.from_appstore = true;
                    current.name          = json_load_value_string(info_json, "title");
                    current.author        = json_load_value_string(info_json, "author");
                    current.category      = json_load_value_string(info_json, "category");
                    current.version       = json_load_value_string(info_json, "version");
                    current.url           = json_load_value_string(info_json, "url");
                    current.license       = json_load_value_string(info_json, "license");
                    current.description   = json_load_value_string(info_json, "description");
                    current.summary       = json_load_value_string(info_json, "details");
                    current.changelog     = json_load_value_string(info_json, "changelog");

                    print_debug((current.name + "\n").c_str());
                }

                std::string manifest_file = folder + "/manifest.install";
                if (fs::exists(manifest_file))
                {
                    print_debug(("manifest: " + manifest_file + "\n").c_str());

                    std::ifstream file(manifest_file);
                    std::string str;
                    while (std::getline(file, str))
                    {
                        if (str.length() > 4 && str.substr(str.length() - 4) == ".nro")
                        {
                            current.manifest_path = "sdmc:/" + str.substr(3, str.length());
                            current.from_appstore = true;
                            break;
                        }
                    }
                }

                if (current.from_appstore)
                    store_file_data.push_back(current);
            }
        }
        sort(store_file_data.begin(), store_file_data.end(), compare_by_name);
    }
}

void process_app_file(std::string filename)
{
    print_debug((filename + "\n").c_str());

    if (filename.length() > 4)
    {
        if (filename.substr(filename.length() - 4) == ".nro")
        {
            print_debug("read:\n");
            app_entry current;
            read_nacp_from_file(filename, &current);
            read_icon_from_file(filename, &current);
            current.from_appstore = false;
            print_debug("nacp and icon okay\n");

            // Check against store apps
            int count = 0;
            for (auto store_entry : store_file_data)
            {
                count++;
                if (to_lower(store_entry.manifest_path) == to_lower(current.full_path))
                {
                    current.from_appstore = true;
                    current.category      = store_entry.category;
                    current.url           = store_entry.url;
                    current.license       = store_entry.license;
                    current.description   = store_entry.description;
                    current.summary       = store_entry.summary;
                    current.changelog     = store_entry.changelog;
                    current.manifest_path = store_entry.manifest_path;

                    store_apps.push_back(store_entry);
                    store_file_data.erase(store_file_data.begin() + count);
                    break;
                }
            }

            current.favorite = vector_contains(favorites, current.full_path);

            print_debug("done with stores\n");

            if (!current.name.empty())
                local_apps.push_back(current);
        }
    }
}

void list_files(const char* basePath, bool recursive)
{
    char path[1000];
    struct dirent* dp;
    DIR* dir = opendir(basePath);

    if (!dir)
        return;

    // blacklist
    std::string pa = to_lower(std::string(basePath));
    string_replace(pa, "//", "/");
    if (vector_contains(blacklist, pa))
    {
        print_debug("Blacklist: " + pa + " = " + "sdmc:/retroarch\n");
    }
    else
    {
        while ((dp = readdir(dir)) != NULL)
        {
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
            {
                std::string filename = std::string(basePath) + "/" + std::string(dp->d_name);
                file_count += 1;
                last_file_name = filename;

                string_replace(filename, "//", "/");
                if (filename.length() > 3 && filename.substr(filename.length() - 4) == ".nro")
                {
                    process_app_file(filename);
                }
                else
                    print_debug("Skip: " + filename + "\n");

                // Construct new path from our base path
                strcpy(path, basePath);
                strcat(path, "/");
                strcat(path, dp->d_name);

                if (scanprog.end_thread)
                    break;

                if (recursive)
                    list_files(path, true);
            }
        }
    }

    closedir(dir);
}

void load_all_apps()
{
    local_apps.clear();

    if (get_setting_true(setting_scan_full_card))
    {
        print_debug("Searching recursively within /\n");
        list_files("sdmc:/", true);
    }
    else
    {
        if (get_setting_true(setting_search_subfolders))
        {
            print_debug("Searching recursively within /switch/\n");
            list_files("sdmc:/switch", true);
        }
        else
        {
            print_debug("Searching only within /switch/\n");
            list_files("sdmc:/switch", false);
        }

        if (get_setting_true(setting_search_root))
        {
            print_debug("Searching only within /\n");
            list_files("sdmc:/", false);
        }
    }

    store_file_data.clear();
    if (local_apps.size() > 1)
        sort(local_apps.begin(), local_apps.end(), compare_by_name);
}