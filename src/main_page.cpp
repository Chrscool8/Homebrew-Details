#include "main_page.hpp"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include <algorithm>
#include <borealis.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "intro_page.hpp"

namespace fs = std::filesystem;

struct app_entry
{
    std::string name;
    std::string file_name;
    std::string full_path;
    std::string author;
    std::string version;
    std::size_t size;
    std::size_t icon_size;
    uint8_t* icon;
    bool from_appstore;
    std::string url;
    std::string category;
    std::string license;
    std::string description;
    std::string summary;
    std::string changelog;
};

std::vector<app_entry> switch_apps;
std::vector<app_entry> store_apps;

std::string json_load_value_string(nlohmann::json json, std::string key)
{
    if (json.contains(key))
        return json[key].get<std::string>();
    else
        return "---";
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

void read_store_apps()
{
    store_apps.clear();

    std::string path = "/switch/appstore/.get/packages/";
    if (fs::exists(path))
    {
        for (const auto& entry : fs::directory_iterator(path))
        {
            std::string folder = entry.path();
            if (fs::is_directory(folder))
            {
                //printf(("folder: " + folder + "\n").c_str());

                std::string info_file = folder + "/info.json";
                if (fs::exists(info_file))
                {
                    //printf(("info_file: " + info_file + "\n").c_str());

                    std::ifstream i(info_file);
                    nlohmann::json info_json;
                    i >> info_json;

                    app_entry current;
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

                    if (!current.name.empty())
                        store_apps.push_back(current);

                    //printf((current.name + "\n").c_str());
                }
            }
        }
        sort(store_apps.begin(), store_apps.end(), compare_by_name);
    }
}

void read_nacp_from_file(std::string path, app_entry* current)
{
    FILE* file = fopen(path.c_str(), "rb");
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

        fclose(file);

        current->name      = name;
        current->author    = author;
        current->version   = version;
        current->file_name = path.substr(path.find_last_of("/\\") + 1);
        current->full_path = path;
        current->size      = fs::file_size(path);
    }
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

void load_all_apps()
{
    switch_apps.clear();

    std::string path = "/switch/";
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        std::string filename = entry.path();
        if (filename.substr(filename.length() - 4) == ".nro")
        {
            app_entry current;
            read_nacp_from_file(filename, &current);
            read_icon_from_file(filename, &current);
            current.from_appstore = false;

            // Check against store apps
            int count = 0;
            for (auto store_entry : store_apps)
            {
                count++;
                if (store_entry.name != current.name || store_entry.version != current.version)
                { }
                else
                {
                    current.from_appstore = true;
                    //current.author        = store_entry.author;
                    current.category    = store_entry.category;
                    current.url         = store_entry.url;
                    current.license     = store_entry.license;
                    current.description = store_entry.description;
                    current.summary     = store_entry.summary;
                    current.changelog   = store_entry.changelog;

                    store_apps.erase(store_apps.begin() + count);
                    break;
                }
            }

            if (!current.name.empty())
                switch_apps.push_back(current);
        }
    }

    sort(switch_apps.begin(), switch_apps.end(), compare_by_name);
}

brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to)
{
    brls::ListItem* item = new brls::ListItem(title);

    const int clip_length = 21;
    if (short_info.length() > clip_length)
    {
        if (long_info.empty())
            long_info = "Full " + title + ":\n\n" + short_info;
        short_info = short_info.substr(0, 21) + "[...]";
    }

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

    add_to->addView(item);
    return item;
}

brls::ListItem* make_app_entry(app_entry* entry)
{
    brls::ListItem* popupItem = new brls::ListItem(entry->name);
    popupItem->setValue("v" + entry->version);
    popupItem->setThumbnail(entry->icon, entry->icon_size);
    popupItem->getClickEvent()->subscribe([entry](brls::View* view) mutable {
        brls::TabFrame* appView = new brls::TabFrame();
        brls::List* appInfoList = new brls::List();

        appInfoList->addView(new brls::Header(".NRO File Info", false));

        add_list_entry("Name", entry->name, "", appInfoList);
        add_list_entry("Filename", entry->file_name, "Full Path:\n\nsdmc:" + entry->full_path, appInfoList);
        add_list_entry("Author", entry->author, "", appInfoList);
        add_list_entry("Version", entry->version, "", appInfoList);
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << entry->size / 1000. / 1000.;
        std::string str = stream.str();
        add_list_entry("Size", str + " MB", "Exact Size:\n\n" + std::to_string(entry->size) + " bytes", appInfoList);
        add_list_entry("Icon Size", std::to_string(entry->icon_size), "", appInfoList);
        appView->addTab("File Info", appInfoList);

        brls::List* appStoreInfoList = new brls::List();
        appStoreInfoList->addView(new brls::Header("App Store Info", false));

        add_list_entry("From Appstore", (entry->from_appstore ? "Yes" : "No"), "", appStoreInfoList);
        add_list_entry("URL", entry->url, "", appStoreInfoList);
        add_list_entry("Category", entry->category, "", appStoreInfoList);
        add_list_entry("License", entry->license, "", appStoreInfoList);
        add_list_entry("Description", entry->description, "", appStoreInfoList);
        add_list_entry("Summary", entry->summary, "", appStoreInfoList);
        add_list_entry("Changelog", entry->changelog, "", appStoreInfoList);

        appView->addTab("App Store Info", appStoreInfoList);
        //appView->addTab("Notes", new brls::Rectangle(nvgRGB(120, 120, 120)));

        brls::PopupFrame::open(entry->name, entry->icon, entry->icon_size, appView, "Author: " + entry->author, "Version: " + entry->version);
    });

    return popupItem;
}

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

MainPage::MainPage()
{
    std::string title = "Homebrew Details v" APP_VERSION;

    this->setTitle(title.c_str());
    this->setIcon(BOREALIS_ASSET("icon.jpg"));
    printf("init rootframe");
    this->setActionAvailable(brls::Key::B, false);

    read_store_apps();
    load_all_apps();

    brls::List* appsList      = new brls::List();
    brls::List* storeAppsList = new brls::List();
    brls::List* localAppsList = new brls::List();

    for (unsigned int i = 0; i < switch_apps.size(); i++)
    {
        app_entry* current = &switch_apps.at(i);
        appsList->addView(make_app_entry(current));

        if (current->from_appstore)
            storeAppsList->addView(make_app_entry(current));
        else
            localAppsList->addView(make_app_entry(current));
    }

    this->addTab("All Apps", appsList);
    this->addSeparator();
    if (!store_apps.empty())
        this->addTab("App Store Apps", storeAppsList);
    if (!switch_apps.empty())
        this->addTab("Local Apps", localAppsList);
    //rootFrame->addSeparator();
    //rootFrame->addTab("Applications", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Emulators", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Games", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Tools", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Misc.", new brls::Rectangle(nvgRGB(120, 120, 120)));
    this->addSeparator();

    //this->addTab("Settings", new brls::Rectangle(nvgRGB(120, 120, 120)));
}

MainPage::~MainPage()
{
}
