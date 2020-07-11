#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include <algorithm>
#include <borealis.hpp>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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

void load_all_apps()
{
    std::string path = "/switch/";
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        std::string filename = entry.path();
        if (filename.substr(filename.length() - 4) == ".nro")
        {
            app_entry current;
            read_nacp_from_file(filename, &current);
            read_icon_from_file(filename, &current);
            if (!current.name.empty())
                apps.push_back(current);
        }
    }

    sort(apps.begin(), apps.end(), compare_by_name);
}

brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to)
{
    brls::ListItem* item = new brls::ListItem(title);
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

int main(int argc, char* argv[])
{
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    if (!brls::Application::init("Homebrew Details"))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    brls::TabFrame* rootFrame = new brls::TabFrame();
    rootFrame->setTitle("Homebrew Details");
    rootFrame->setIcon(BOREALIS_ASSET("icon.jpg"));

    load_all_apps();

    brls::List* appsList = new brls::List();

    for (unsigned int i = 0; i < apps.size(); i++)
    {
        app_entry current = apps.at(i);

        brls::ListItem* popupItem = new brls::ListItem(current.name);
        popupItem->setValue("v" + current.version);
        popupItem->setThumbnail(current.icon, current.icon_size);
        popupItem->getClickEvent()->subscribe([current](brls::View* view) mutable {
            brls::TabFrame* appView = new brls::TabFrame();
            brls::List* appInfoList = new brls::List();

            appInfoList->addView(new brls::Header(".NRO File Info", false));

            add_list_entry("Name", current.name, "", appInfoList);
            add_list_entry("Filename", current.file_name, "Full Path:\n\nsdmc:" + current.full_path, appInfoList);
            add_list_entry("Author", current.author, "", appInfoList);
            add_list_entry("Version", current.version, "", appInfoList);
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << current.size / 1000. / 1000.;
            std::string str = stream.str();
            add_list_entry("Size", str + " MB", "Exact Size:\n\n" + std::to_string(current.size) + " bytes", appInfoList);
            add_list_entry("Icon Size", std::to_string(current.icon_size), "", appInfoList);

            appView->addTab("File Info", appInfoList);
            appView->addTab("Notes", new brls::Rectangle(nvgRGB(120, 120, 120)));

            brls::PopupFrame::open(current.name, current.icon, current.icon_size, appView, "Author: " + current.author, "Version: " + current.version);
        });
        appsList->addView(popupItem);
    }

    rootFrame->addTab("All Apps", appsList);
    rootFrame->addSeparator();
    rootFrame->addTab("App Store Apps", new brls::Rectangle(nvgRGB(60, 60, 60)));
    rootFrame->addTab("Local Apps", new brls::Rectangle(nvgRGB(60, 60, 60)));
    rootFrame->addSeparator();
    rootFrame->addTab("Settings", new brls::Rectangle(nvgRGB(120, 120, 120)));

    brls::Application::pushView(rootFrame);

    // Run the app
    while (brls::Application::mainLoop())
        ;

    // Protect from crash
    rootFrame->setParent(NULL);
    return EXIT_SUCCESS;
}