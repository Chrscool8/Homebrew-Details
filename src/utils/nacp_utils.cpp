#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>

//

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __SWITCH__
#include <switch.h>
#include "switch/services/psm.h"
#include <sys/select.h>
#include <sys/stat.h>
#else
#include <extern/nacp_win.h>
#include <extern/nro_win.h>
#include <extern/result_win.h>
#endif // __SWITCH__

//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <borealis.hpp>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

nlohmann::json errored_entry(std::string path)
{
    nlohmann::json app_json;

    std::string name = std::string(path);
    string_replace(name, ".nro", "");
    app_json["name"]      = name.substr(name.find_last_of("/\\") + 1);
    app_json["author"]    = "Unknown";
    app_json["version"]   = "Unknown";
    app_json["full_path"] = path;
    app_json["file_name"] = path.substr(path.find_last_of("/\\") + 1);
    app_json["size"]      = fs::file_size(path);

    return app_json;
}

nlohmann::json read_nacp_from_file(std::string path)
{
    nlohmann::json app_json;

    FILE* file = fopen(path.c_str(), "rb");
    if (file)
    {
        char name[513];
        char author[257];
        char version[17];

        fseek(file, sizeof(NroStart), SEEK_SET);
        NroHeader header;
        size_t sz = fread(&header, sizeof(header), 1, file);
        if (sz != 1)
        {
            fclose(file);
            return errored_entry(path);
        }

        fseek(file, header.size, SEEK_SET);
        NroAssetHeader asset_header;
        if (fread(&asset_header, sizeof(asset_header), 1, file) != 1
            || asset_header.magic != NROASSETHEADER_MAGIC
            || asset_header.version > NROASSETHEADER_VERSION
            || asset_header.nacp.offset == 0
            || asset_header.nacp.size == 0)
        {
            fclose(file);
            return errored_entry(path);
        }

        if (asset_header.nacp.size < sizeof(NacpStruct))
        {
            fclose(file);
            return errored_entry(path);
        }

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

        app_json["name"]      = name;
        app_json["author"]    = author;
        app_json["version"]   = version;
        app_json["full_path"] = path;
        app_json["file_name"] = path.substr(path.find_last_of("/\\") + 1);
        app_json["size"]      = fs::file_size(path);

        fclose(file);
    }

    return app_json;
}

bool read_icon_from_file(std::string path, app_entry* current)
{
    FILE* file = fopen(path.c_str(), "rb");
    if (!file)
        return false;

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

        current->icon      = icon;
        current->icon_size = icon_size;
    }

    fclose(file);
    return true;
}