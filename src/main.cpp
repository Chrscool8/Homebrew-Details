/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019-2020  natinusala
    Copyright (C) 2019  p-sam

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include <borealis.hpp>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "sample_installer_page.hpp"
#include "sample_loading_page.hpp"
namespace fs = std::filesystem;

struct app_entry
{
    std::string name;
    std::string author;
    std::string version;
    std::string full_path;
    std::string file_name;
    std::size_t size;
    std::size_t icon_size;
    uint8_t* icon;
};

std::vector<app_entry> apps;

#define ENTRY_NAMELENGTH 0x200
#define ENTRY_AUTHORLENGTH 0x100
#define ENTRY_VERLENGTH 0x10
#define ENTRY_ARGBUFSIZE 0x400

// from hbmenu
struct menuEntry_s_tag
{
    char path[PATH_MAX + 8];

    bool fileassoc_type; //0=file_extension, 1 = filename
    char fileassoc_str[PATH_MAX + 1]; //file_extension/filename

    char name[ENTRY_NAMELENGTH + 1];
    char author[ENTRY_AUTHORLENGTH + 1];
    char version[ENTRY_VERLENGTH + 1];

    uint8_t* icon;
    size_t icon_size;
    uint8_t* icon_gfx;
    uint8_t* icon_gfx_small;

    bool starred;

    NacpStruct* nacp;
};
typedef struct menuEntry_s_tag menuEntry_s;
app_entry menuEntryLoadEmbeddedNacp(std::string path)
{
    app_entry current;
    menuEntry_s* me = new menuEntry_s();
    NroHeader header;
    NroAssetHeader asset_header;

    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
        return current;

    fseek(f, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(header), 1, f) != 1)
    {
        fclose(f);
        return current;
    }

    fseek(f, header.size, SEEK_SET);

    if (fread(&asset_header, sizeof(asset_header), 1, f) != 1
        || asset_header.magic != NROASSETHEADER_MAGIC
        || asset_header.version > NROASSETHEADER_VERSION
        || asset_header.nacp.offset == 0
        || asset_header.nacp.size == 0)
    {
        fclose(f);
        return current;
    }

    if (asset_header.nacp.size < sizeof(NacpStruct))
    {
        fclose(f);
        return current;
    }

    me->nacp = (NacpStruct*)malloc(sizeof(NacpStruct));
    if (me->nacp == NULL)
    {
        fclose(f);
        return current;
    }

    fseek(f, header.size + asset_header.nacp.offset, SEEK_SET);
    fread(me->nacp, sizeof(NacpStruct), 1, f);
    fclose(f);

    NacpLanguageEntry* langentry = NULL;

    if (me->nacp == NULL)
    {
        current.author = "nacp = null";
        return current;
    }

    strncpy(me->version, me->nacp->display_version, sizeof(me->version) - 1);

#ifdef __SWITCH__
    Result rc = 0;
    rc        = nacpGetLanguageEntry(me->nacp, &langentry);

    if (R_SUCCEEDED(rc) && langentry != NULL)
    {
#else
    langentry = &me->nacp->lang[0];
    if (1)
    {
#endif
        strncpy(me->name, langentry->name, sizeof(me->name) - 1);
        strncpy(me->author, langentry->author, sizeof(me->author) - 1);
    }

    free(me->nacp);
    me->nacp = NULL;

    current.name      = me->name;
    current.author    = me->author;
    current.version   = me->version;
    current.file_name = path.substr(path.find_last_of("/\\") + 1);
    current.full_path = path;
    current.size      = fs::file_size(path);
    return current;
}

bool menuEntryLoadEmbeddedIcon(std::string path, app_entry* current)
{
    menuEntry_s* me = new menuEntry_s();
    NroHeader header;
    NroAssetHeader asset_header;

    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
        return false;

    fseek(f, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(header), 1, f) != 1)
    {
        fclose(f);
        return false;
    }

    fseek(f, header.size, SEEK_SET);

    if (fread(&asset_header, sizeof(asset_header), 1, f) != 1
        || asset_header.magic != NROASSETHEADER_MAGIC
        || asset_header.version > NROASSETHEADER_VERSION
        || asset_header.icon.offset == 0
        || asset_header.icon.size == 0)
    {
        fclose(f);
        return false;
    }

    me->icon_size = asset_header.icon.size;
    me->icon      = (uint8_t*)malloc(me->icon_size);
    if (me->icon == NULL)
    {
        fclose(f);
        return false;
    }
    memset(me->icon, 0, me->icon_size);

    fseek(f, header.size + asset_header.icon.offset, SEEK_SET);
    bool ok = fread(me->icon, me->icon_size, 1, f) == 1;
    fclose(f);

    current->icon      = me->icon;
    current->icon_size = me->icon_size;
    return ok;
}

bool compare_by_name(const app_entry& a, const app_entry& b)
{
    if (a.name != b.name)
        return (a.name.compare(b.name) < 0);
    else
        return (a.version.compare(b.version));
}

void load_apps()
{
    std::string path = "/switch/";
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        std::string filename = entry.path();
        if (filename.substr(filename.length() - 4) == ".nro")
        {
            app_entry current = menuEntryLoadEmbeddedNacp(filename);
            menuEntryLoadEmbeddedIcon(filename, &current);
            if (current.name.length() != 0)
                apps.push_back(current);
        }
    }

    sort(apps.begin(), apps.end(), compare_by_name);
}

brls::ListItem* add_list_entry(std::string name, std::string info, brls::List* add_to)
{
    brls::ListItem* item = new brls::ListItem(name);
    item->setValue(info);
    add_to->addView(item);
    return item;
}

int main(int argc, char* argv[])
{
    // Init the app
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    if (!brls::Application::init("Homebrew Details"))
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    brls::TabFrame* rootFrame = new brls::TabFrame();
    rootFrame->setTitle("Homebrew Details");
    rootFrame->setIcon(BOREALIS_ASSET("icon.jpg"));

    load_apps();

    brls::List* testList = new brls::List();

    for (unsigned int i = 0; i < apps.size(); i++)
    {
        app_entry current = apps.at(i);

        brls::ListItem* popupItem = new brls::ListItem(current.name);
        popupItem->setValue("v" + current.version);
        popupItem->setThumbnail(current.icon, current.icon_size);
        popupItem->getClickEvent()->subscribe([current](brls::View* view) mutable {
            brls::TabFrame* popupTabFrame = new brls::TabFrame();
            brls::List* infoList          = new brls::List();

            infoList->addView(new brls::Header(".NRO File Info", false));

            add_list_entry("Name", current.name, infoList);

            brls::ListItem* f = add_list_entry("Filename", current.file_name, infoList);
            f->getClickEvent()->subscribe([current](brls::View* view) {
                brls::Dialog* dialog                       = new brls::Dialog("Full Path:\n\nsdmc:" + current.full_path);
                brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                    dialog->close();
                };
                dialog->addButton("Dismiss", closeCallback);
                dialog->setCancelable(true);
                dialog->open();
            });
            f->updateActionHint(brls::Key::A, "Show Full Path");

            add_list_entry("Author", current.author, infoList);
            add_list_entry("Version", current.version, infoList);
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << current.size / 1000. / 1000.;
            std::string str = stream.str();
            add_list_entry("Size", str + " MB", infoList);
            add_list_entry("Icon Size", std::to_string(current.icon_size), infoList);

            popupTabFrame->addTab("File Info", infoList);

            brls::PopupFrame::open(current.name, current.icon, current.icon_size, popupTabFrame, "Author: " + current.author, "Version: " + current.version);
        });
        testList->addView(popupItem);
    }

    rootFrame->addTab("All Apps", testList);
    rootFrame->addSeparator();
    rootFrame->addTab("App Store Apps", new brls::Rectangle(nvgRGB(60, 60, 60)));
    rootFrame->addTab("Local Apps", new brls::Rectangle(nvgRGB(60, 60, 60)));
    rootFrame->addSeparator();
    rootFrame->addTab("Settings", new brls::Rectangle(nvgRGB(120, 120, 120)));

    brls::Application::pushView(rootFrame);

    // Run the app
    while (brls::Application::mainLoop())
        ;

    rootFrame->setParent(NULL);

    // Exit
    return EXIT_SUCCESS;
}