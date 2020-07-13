#include "main_page.hpp"

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include "switch/services/psm.h"

//
#include <sys/select.h>
//
#include <curl/curl.h>
#include <curl/easy.h>
//
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

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

namespace fs = std::filesystem;

std::string online_version = "";

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

void purge_entry(app_entry* entry)
{
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

        brls::List* manageList = new brls::List();
        manageList->addView(new brls::Header("File Management Actions", false));

        brls::ListItem* delete_item = new brls::ListItem("Delete App");
        delete_item->getClickEvent()->subscribe([entry, appView](brls::View* view) {
            brls::Dialog* dialog                     = new brls::Dialog("Are you sure you want to delete the following file? This action cannot be undone.\n\n" + entry->full_path);
            brls::GenericEvent::Callback yesCallback = [dialog, entry, appView](brls::View* view) {
                if (remove(entry->full_path.c_str()) != 0)
                    brls::Application::notify("Issue removing file");
                else
                {
                    brls::Application::notify("File successfully deleted");
                    purge_entry(entry);
                }

                dialog->close();
            };
            brls::GenericEvent::Callback noCallback = [dialog](brls::View* view) {
                dialog->close();
            };
            dialog->addButton("!!  [Yes]  !!", yesCallback);
            dialog->addButton("No", noCallback);
            dialog->setCancelable(true);
            dialog->open();
        });
        //delete_item->updateActionHint(brls::Key::A, "Show Extended Info");

        manageList->addView(delete_item);

        appView->addTab("Manage", manageList);

        //appView->addTab("Notes", new brls::Rectangle(nvgRGB(120, 120, 120)));

        brls::PopupFrame::open(entry->name, entry->icon, entry->icon_size, appView, "Author: " + entry->author, "Version: " + entry->version);
    });

    return popupItem;
}

size_t CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

bool is_number(const std::string& s)
{
    return (strspn(s.c_str(), "-.0123456789") == s.size() && s.length() > 0);
}

bool check_for_updates()
{
    printf("curl time\n");

    CURL* curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        std::string s;

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/Chrscool8/Homebrew-Details/releases/latest");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Homebrew-Details");

        CURLcode res;
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            nlohmann::json j = nlohmann::json::parse(s);
            if (j.contains("tag_name"))
            {
                online_version = j["tag_name"].get<std::string>();

                if (online_version.length() > 0)
                {
                    online_version = online_version.substr(1);
                }

                if (is_number(online_version))
                {
                    printf((std::string("") + online_version + " : " + APP_VERSION + "\n").c_str());

                    if (std::stod(online_version) > std::stod(APP_VERSION))
                    {
                        return true;
                    }
                }
            }
        }
    }

    //brls::Application::notify("problem parsing online version\n");
    return false;
}

MainPage::MainPage()
{
    std::string title = "Homebrew Details v" APP_VERSION;

    this->setTitle(title.c_str());
    this->setIcon(BOREALIS_ASSET("icon.jpg"));
    printf("init rootframe");
    //this->setActionAvailable(brls::Key::B, false);

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

    this->addTab("All Apps               (" + std::to_string(store_apps.size() + switch_apps.size()) + ")", appsList);
    this->addSeparator();
    if (!store_apps.empty())
        this->addTab("App Store Apps     (" + std::to_string(store_apps.size()) + ")", storeAppsList);
    if (!switch_apps.empty())
        this->addTab("Local Apps            (" + std::to_string(switch_apps.size()) + ")", localAppsList);

    //rootFrame->addSeparator();
    //rootFrame->addTab("Applications", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Emulators", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Games", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Tools", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Misc.", new brls::Rectangle(nvgRGB(120, 120, 120)));

    psmInitialize();
    std::uint32_t batteryCharge = 0;
    psmGetBatteryChargePercentage(&batteryCharge);

    //this->addTab("Read: "+std::to_string(batteryCharge), new brls::Rectangle(nvgRGB(120, 120, 120)));

    if (check_for_updates())
    {
        this->addSeparator();
        brls::List* settingsList = new brls::List();
        settingsList->addView(new brls::Header("Newer Version Found Online", false));

        brls::ListItem* dialogItem = new brls::ListItem("More Info...");
        dialogItem->getClickEvent()->subscribe([](brls::View* view) {
            brls::Dialog* dialog = new brls::Dialog(std::string("") + "You have v" + APP_VERSION + " but v" + online_version + " is out.\n\n The auto-updater isn't ready quite yet, but you can find the new version in the GBATemp forum topic or my Github.");

            brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                dialog->close();
            };
            brls::GenericEvent::Callback infoCallback = [dialog](brls::View* view) {
                //dialog->close();

                brls::Dialog* dialog1 = new brls::Dialog(std::string("") + "GBATemp Discussion Topic:\nhttps://gbatemp.net/threads/homebrew-details-a-homebrew-app-manager.569528/\n\nGithub Repo:\nhttps://github.com/Chrscool8/Homebrew-Details");

                brls::GenericEvent::Callback closeCallback1 = [dialog1](brls::View* view) {
                    dialog1->close();
                };

                dialog1->addButton("Okay.", closeCallback1);
                dialog1->setCancelable(true);

                dialog1->open();
            };

            dialog->addButton("Okay.", closeCallback);
            dialog->addButton("Link Info.", infoCallback);
            dialog->setCancelable(true);

            dialog->open();
        });
        settingsList->addView(dialogItem);
        this->addTab("Update Available!", settingsList);
    }
}

MainPage::~MainPage()
{
}
