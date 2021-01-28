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

int isDirectory(const char* path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

void new_read_store_apps()
{
    const char* basePath = "/switch/appstore/.get/packages/";

    char path[1000];
    struct dirent* dp;
    DIR* dir = opendir(basePath);

    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            std::string filename = std::string(basePath) + "/" + std::string(dp->d_name);

            string_replace(filename, "//", "/");

            if (isDirectory(filename.c_str()))
            {
                std::ifstream info_file(filename + "/info.json");
                nlohmann::json app_info_json = nlohmann::json::parse(info_file);

                std::ifstream manifest_file(filename + "/manifest.install");
                std::vector<std::string> files_in_manifest;
                std::string file_line;
                while (std::getline(manifest_file, file_line))
                {
                    if (file_line.size() > 0)
                    {
                        files_in_manifest.push_back(file_line.substr(3));

                        if (apps_info_json.contains("sdmc:/" + file_line.substr(3)))
                        {
                            print_debug(file_line.substr(3) + " found in " + immediate_folder_of_file(filename));
                            nlohmann::json temp_json = apps_info_json[std::string("sdmc:/" + file_line.substr(3))];

                            temp_json["description"]   = app_info_json["description"];
                            temp_json["store author"]  = app_info_json["author"];
                            temp_json["changelog"]     = app_info_json["changelog"];
                            temp_json["category"]      = app_info_json["category"];
                            temp_json["store title"]   = app_info_json["title"];
                            temp_json["license"]       = app_info_json["license"];
                            temp_json["store version"] = app_info_json["version"];
                            temp_json["url"]           = app_info_json["url"];
                            temp_json["is_appstore"]   = "true";

                            apps_info_json[std::string("sdmc:/" + file_line.substr(3))] = temp_json;
                        }
                    }
                }

                app_info_json["files"] = files_in_manifest;

                store_info_json[immediate_folder_of_file(filename)] = app_info_json;
            }
        }
    }

    closedir(dir);

    //std::ofstream o(get_config_path("temp_store_info.json"));
    //o << store_info_json << std::endl;

    std::ofstream o2(get_config_path() + "apps_info.json");
    o2 << apps_info_json << std::endl;
}

void new_list_files(const char* basePath, bool recursive)
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
        print_debug("Blacklist: " + pa);
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
                    print_debug("Found: " + filename);

                    //

                    FILE* file = fopen(filename.c_str(), "rb");
                    if (file)
                    {
                        char name[513];
                        char author[257];
                        char version[17];

                        fseek(file, sizeof(NroStart), SEEK_SET);
                        NroHeader header;
                        fread(&header, sizeof(header), 1, file);
                        fseek(file, header.size, SEEK_SET);
                        NroAssetHeader asset_header;
                        fread(&asset_header, sizeof(asset_header), 1, file);

                        NacpStruct* nacp = (NacpStruct*)malloc(sizeof(NacpStruct));
                        if (nacp != NULL)
                        {
                            fseek(file, header.size + asset_header.nacp.offset, SEEK_SET);
                            fread(nacp, sizeof(NacpStruct), 1, file);

                            NacpLanguageEntry* langentry = NULL;
                            Result rc                    = nacpGetLanguageEntry(nacp, &langentry);
                            if (R_SUCCEEDED(rc) && langentry != NULL)
                            {
                                strncpy(name, langentry->name, sizeof(name) - 1);
                                strncpy(author, langentry->author, sizeof(author) - 1);
                            }
                            strncpy(version, nacp->display_version, sizeof(version) - 1);

                            free(nacp);
                            nacp = NULL;
                        }

                        nlohmann::json app_json;

                        app_json["name"]      = name;
                        app_json["author"]    = author;
                        app_json["version"]   = version;
                        app_json["full_path"] = filename;
                        app_json["file_name"] = filename.substr(filename.find_last_of("/\\") + 1);
                        app_json["size"]      = fs::file_size(filename);

                        apps_info_json[filename] = app_json;

                        //size_t icon_size = asset_header.icon.size;
                        //uint8_t* icon    = (uint8_t*)malloc(icon_size);
                        //if (icon != NULL && icon_size != 0)
                        //{
                        //    memset(icon, 0, icon_size);
                        //    fseek(file, header.size + asset_header.icon.offset, SEEK_SET);
                        //    fread(icon, icon_size, 1, file);

                        //    create_directories(get_config_path("cache/"));
                        //    std::ofstream fp;
                        //    fp.open((std::string(get_config_path("cache/")) + base64_encode(std::string(name) + version) + ".jpg").c_str(), std::ios::out | std::ios::binary);
                        //    fp.write((char*)icon, icon_size);
                        //    fp.close();
                        //}

                        //free(icon);
                        //icon = NULL;

                        fclose(file);
                    }

                    //
                }
                //else
                //    print_debug("Skip: " + filename);

                // Construct new path from our base path
                strcpy(path, basePath);
                strcat(path, "/");
                strcat(path, dp->d_name);

                if (scanprog.end_thread)
                    break;

                if (recursive)
                    new_list_files(path, true);
            }
        }
    }

    closedir(dir);
}

void new_load_all_apps()
{
    if (settings_get_value_true("scan", "full card"))
    {
        print_debug("Searching recursively within /");
        new_list_files("sdmc:/", true);
    }
    else
    {
        if (settings_get_value_true("scan", "subfolders"))
        {
            print_debug("Searching recursively within /switch/");
            new_list_files("sdmc:/switch", true);
        }
        else
        {
            print_debug("Searching only within /switch/");
            new_list_files("sdmc:/switch", false);
        }

        if (settings_get_value_true("scan", "root"))
        {
            print_debug("Searching only within /");
            new_list_files("sdmc:/", false);
        }
    }

    std::ofstream o(get_config_path() + "apps_info.json");
    o << apps_info_json << std::endl;
}

brls::Image* load_image_cache(std::string filename)
{
    print_debug("Requesting " + filename);

    brls::Image* image = nullptr;

    std::string filename_enc = base64_encode(filename);

    std::map<std::string, brls::Image*>::iterator it;

    it = cached_thumbs.find(filename_enc);
    // found
    if (it != cached_thumbs.end())
    {
        print_debug(" Already Cached");
        image = cached_thumbs[filename_enc];
    }
    else
    // not found
    {
        print_debug(" Not yet cached");

        FILE* file = fopen(filename.c_str(), "rb");
        if (file)
        {
            fseek(file, sizeof(NroStart), SEEK_SET);
            NroHeader header;
            fread(&header, sizeof(header), 1, file);
            fseek(file, header.size, SEEK_SET);
            NroAssetHeader asset_header;
            fread(&asset_header, sizeof(asset_header), 1, file);

            size_t icon_size = asset_header.icon.size;
            uint8_t* icon    = (uint8_t*)malloc(icon_size);
            if (icon != NULL && icon_size != 0)
            {
                memset(icon, 0, icon_size);
                fseek(file, header.size + asset_header.icon.offset, SEEK_SET);
                fread(icon, icon_size, 1, file);

                print_debug(" Caching new");

                image = new brls::Image(icon, icon_size);

                cached_thumbs[filename_enc] = image;
            }
            else
                image = new brls::Image(get_resource_path() + "unknown.png");

            free(icon);
            icon = NULL;
        }
        else
        {
            print_debug(" Using Unknown");
            image = new brls::Image(get_resource_path() + "unknown.png");
        }
    }

    return image;
}