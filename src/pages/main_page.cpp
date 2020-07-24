#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>
#include <pages/update_page.hpp>
//

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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

namespace fs = std::filesystem;

void MainPage::read_favorites()
{
    std::ifstream inputFile("sdmc:/config/homebrew_details/favorites.txt");
    if (inputFile)
    {
        int index = 0;
        while (inputFile)
        {
            char line[513];
            inputFile.getline(line, 512);
            favorites.push_back(base64_decode(line));
            print_debug(line);
            print_debug("\n");
            index += 1;
        }
        inputFile.close();
    }
    else
        print_debug("Can't find favorites file.\n");
}

void MainPage::write_favorites()
{
    if (!fs::exists("sdmc:/config/"))
        fs::create_directory("sdmc:/config/");
    if (!fs::exists("sdmc:/config/homebrew_details/"))
        fs::create_directory("sdmc:/config/homebrew_details/");

    remove("sdmc:/config/homebrew_details/favorites.txt");
    std::ofstream outputFile("sdmc:/config/homebrew_details/favorites.txt");
    if (outputFile)
    {
        unsigned int index = 0;
        while (index < favorites.size())
        {
            outputFile << (base64_encode(favorites.at(index))).c_str() << std::endl;
            index += 1;
        }
        outputFile.close();
    }
    else
        print_debug("Can't open favorites.\n");
}

void MainPage::add_favorite(std::string str)
{
    favorites.push_back(str);
    write_favorites();
}

void MainPage::remove_favorite(std::string str)
{
    favorites.erase(std::remove(favorites.begin(), favorites.end(), str), favorites.end());
    write_favorites();
}

void MainPage::read_store_apps()
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

                std::string info_file = folder + "/info.json";
                if (fs::exists(info_file))
                {
                    print_debug(("info_file: " + info_file + "\n").c_str());

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
                        store_file_data.push_back(current);

                    print_debug((current.name + "\n").c_str());
                }
            }
        }
        sort(store_file_data.begin(), store_file_data.end(), compare_by_name);
    }
}

void MainPage::process_app_file(std::string filename)
{
    print_debug((filename + "\n").c_str());

    if (filename.length() > 3)
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
                if (store_entry.name != current.name || store_entry.version != current.version)
                { }
                else
                {
                    //current.author        = store_entry.author;
                    current.from_appstore = true;
                    current.category      = store_entry.category;
                    current.url           = store_entry.url;
                    current.license       = store_entry.license;
                    current.description   = store_entry.description;
                    current.summary       = store_entry.summary;
                    current.changelog     = store_entry.changelog;

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

void MainPage::load_all_apps()
{
    local_apps.clear();

    try //icky
    {
        if (get_setting(setting_scan_full_card) == "true")
        {
            print_debug("Searching recursively within /\n");
            for (const auto& entry : fs::recursive_directory_iterator("/"))
            {
                if (fs::is_regular_file(entry))
                    process_app_file(entry.path());
            }
        }
        else
        {
            if (get_setting(setting_search_subfolders) == "true")
            {
                print_debug("---------------\n");
                print_debug("Searching recursively within /switch/\n");

                // search base folder
                std::vector<std::string> folders;
                for (const auto& entry : fs::directory_iterator("/switch/"))
                {
                    print_debug(entry.path());
                    print_debug("\n");
                    std::string path_str = entry.path();

                    if (fs::is_directory(entry))
                    {
                        transform(path_str.begin(), path_str.end(), path_str.begin(), ::tolower);
                        if (path_str != "/switch/checkpoint" && path_str != "/switch/appstore")
                        {
                            path_str += "/";
                            folders.push_back(path_str);
                        }
                    }
                    else
                        process_app_file(path_str);
                }

                if (!folders.empty())
                {
                    for (unsigned int i = 0; i < folders.size(); i++)
                    {
                        // print_debug("Searching within " + folders.at(i) + "\n");
                        for (const auto& entry : fs::recursive_directory_iterator(folders.at(i)))
                        {
                            process_app_file(entry.path());
                        }
                    }
                }

                folders.clear();

                //for (const auto& entry : fs::recursive_directory_iterator("/switch/"))
                //{
                //    print_debug("entry\n");
                //    if (fs::is_regular_file(entry))
                //        process_app_file(entry.path());
                //}
            }
            else
            {
                print_debug("Searching only within /switch/\n");
                for (const auto& entry : fs::directory_iterator("/switch/"))
                {
                    if (fs::is_regular_file(entry))
                        process_app_file(entry.path());
                }
            }

            if (get_setting(setting_search_root) == "true")
            {
                print_debug("Searching only within /\n");
                for (const auto& entry : fs::directory_iterator("/"))
                {
                    if (fs::is_regular_file(entry))
                        process_app_file(entry.path());
                }
            }
        }
    }
    catch (...)
    {
        print_debug("There was a problem with the search!\n");
    }

    store_file_data.clear();
    if (local_apps.size() > 1)
        sort(local_apps.begin(), local_apps.end(), compare_by_name);
}

brls::ListItem* MainPage::add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length = 21)
{
    brls::ListItem* item = new brls::ListItem(title);

    if (short_info.length() > (unsigned int)clip_length)
    {
        if (long_info.empty())
            long_info = "Full " + title + ":\n\n" + short_info;
        short_info = short_info.substr(0, clip_length) + "[...]";
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

brls::ListItem* MainPage::make_app_entry(app_entry* entry)
{
    std::string label = entry->name;

    if (entry->favorite)
    {
        //popupItem->setChecked(true);
        label = "\u2606 " + label;
    }

    brls::ListItem* popupItem = new brls::ListItem(label, "", entry->full_path);
    popupItem->setValue("v" + entry->version);
    popupItem->setThumbnail(entry->icon, entry->icon_size);

    popupItem->registerAction("Favorite", brls::Key::X, [this, entry, popupItem]() {
        if (vector_contains(favorites, entry->full_path))
        {
            remove_favorite(entry->full_path);
            entry->favorite = false;
            popupItem->setChecked(false);
        }
        else
        {
            add_favorite(entry->full_path);
            entry->favorite = true;
            popupItem->setChecked(true);
        }

        return true;
    });

    popupItem->getClickEvent()->subscribe([this, entry](brls::View* view) mutable {
        brls::TabFrame* appView = new brls::TabFrame();

        brls::List* manageList = new brls::List();
        manageList->addView(new brls::Header("File Management Actions", false));

        if (get_setting_true(setting_debug))
        {
            brls::ListItem* launch_item = new brls::ListItem("Launch App (Beta)");
            launch_item->getClickEvent()->subscribe([this, entry](brls::View* view) {
                print_debug("launch app\n");
                unsigned int r = launch_nro("sdmc:" + entry->full_path, "");
                print_debug("r: " + std::to_string(r) + "\n");
                if (R_FAILED(r))
                {
                    print_debug("Uh oh.\n");
                }
                else
                {
                    local_apps.clear();
                    store_apps.clear();
                    store_file_data.clear();
                    romfsExit();
                    brls::Application::quit();
                }
            });
            manageList->addView(launch_item);
        }

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

void MainPage::build_main_tabs()
{
    brls::List* appsList      = new brls::List();
    brls::List* storeAppsList = new brls::List();
    brls::List* localAppsList = new brls::List();

    for (unsigned int i = 0; i < this->local_apps.size(); i++)
    {
        app_entry* current = &this->local_apps.at(i);
        appsList->addView(make_app_entry(current));

        if (current->from_appstore)
            storeAppsList->addView(make_app_entry(current));
        else
            localAppsList->addView(make_app_entry(current));
    }

    if (!this->local_apps.empty() && !this->store_apps.empty())
    {
        this->addTab(pad_string_with_spaces("All Apps", this->store_apps.size() + this->local_apps.size(), 20).c_str(), appsList);
        this->addSeparator();
    }
    if (!this->store_apps.empty())
        this->addTab(pad_string_with_spaces("App Store Apps", this->store_apps.size(), 9).c_str(), storeAppsList);
    if (!this->local_apps.empty())
        this->addTab(pad_string_with_spaces("Local Apps", this->local_apps.size(), 16).c_str(), localAppsList);
}

MainPage::MainPage()
{
    std::string title = "Homebrew Details v" + get_setting(setting_local_version);
    if (get_setting_true(setting_debug))
        title += " [Debug Mode]";

    this->setTitle(title.c_str());
    this->setIcon(BOREALIS_ASSET("icon.jpg"));
    print_debug("init rootframe\n");
    //this->setActionAvailable(brls::Key::B, false);

    read_favorites();
    read_store_apps();
    load_all_apps();

    build_main_tabs();

    //rootFrame->addSeparator();
    //rootFrame->addTab("Applications", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Emulators", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Games", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Tools", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Misc.", new brls::Rectangle(nvgRGB(120, 120, 120)));

    //this->addTab("Read: "+std::to_string(batteryCharge), new brls::Rectangle(nvgRGB(120, 120, 120)));

    print_debug("Check for updates.\n");
    if (get_online_version_available())
    {
        this->addSeparator();
        brls::List* settingsList = new brls::List();
        settingsList->addView(new brls::Header("Newer Version Found Online", false));

        add_list_entry("Online Version", get_online_version_number(), "", settingsList, 40);
        add_list_entry("Title", get_online_version_name(), "", settingsList, 40);
        add_list_entry("Description", get_online_version_description(), "", settingsList, 40);

        brls::ListItem* dialogItem = new brls::ListItem("Update Now...");
        dialogItem->getClickEvent()->subscribe([this](brls::View* view) {
            brls::Dialog* version_compare_dialog = new brls::Dialog(std::string("") + "You have v" + get_setting(setting_local_version) + " but v" + get_online_version_number() + " is out.\n\nWould you like to download the newest version?");

            brls::GenericEvent::Callback downloadCallback = [this](brls::View* view) {
                brls::Application::pushView(new UpdatePage());
            };

            brls::GenericEvent::Callback infoCallback = [](brls::View* view) {
                brls::Dialog* link_info_dialog = new brls::Dialog(std::string("") + "GBATemp Discussion Topic:\nhttps://gbatemp.net/threads/homebrew-details-a-homebrew-app-manager.569528/\n\nGithub Repo:\nhttps://github.com/Chrscool8/Homebrew-Details");

                brls::GenericEvent::Callback closeCallback1 = [link_info_dialog](brls::View* view) {
                    link_info_dialog->close();
                };

                link_info_dialog->addButton("Okay.", closeCallback1);
                link_info_dialog->setCancelable(true);
                link_info_dialog->open();
            };

            version_compare_dialog->addButton("Yes", downloadCallback);
            version_compare_dialog->addButton("More Info", infoCallback);
            version_compare_dialog->setCancelable(true);

            version_compare_dialog->open();
        });
        settingsList->addView(dialogItem);

        this->addTab("Update Available!", settingsList);
    }

    print_debug("Toolbox.\n");
    {
        this->addSeparator();

        brls::List* tools_list   = new brls::List();
        brls::ListItem* rtp_item = new brls::ListItem("Reboot to Payload");
        rtp_item->setValue("atmosphere/reboot_payload.bin");
        rtp_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("reboot_to_payload\n");
            int result = reboot_to_payload();
            if (result == -1)
                brls::Application::notify("Problem initializing spl");
            else if (result == -2)
                brls::Application::notify("Failed to open atmosphere/ reboot_payload.bin!");
        });
        tools_list->addView(rtp_item);
        this->addTab("Toolbox", tools_list);
    }

    print_debug("Settings.\n");
    {
        brls::List* settings_list = new brls::List();
        settings_list->addView(new brls::Header("Scan Settings"));

        brls::ListItem* autoscan_switch = new brls::ListItem("Autoscan");
        autoscan_switch->setChecked((get_setting(setting_autoscan) == "true"));
        autoscan_switch->updateActionHint(brls::Key::A, "Toggle");
        autoscan_switch->getClickEvent()->subscribe([autoscan_switch](brls::View* view) {
            if (get_setting(setting_autoscan) == "true")
            {
                set_setting(setting_autoscan, "false");
                autoscan_switch->setChecked(false);
            }
            else
            {
                set_setting(setting_autoscan, "true");
                autoscan_switch->setChecked(true);
            }
        });
        settings_list->addView(autoscan_switch);

        brls::ListItem* item_scan_switch = new brls::ListItem("Scan /switch/");
        item_scan_switch->setChecked(true);
        brls::ListItem* item_scan_switch_subs = new brls::ListItem("Scan /switch/'s subfolders");
        item_scan_switch_subs->setChecked((get_setting(setting_search_subfolders) == "true"));
        item_scan_switch_subs->updateActionHint(brls::Key::A, "Toggle");
        item_scan_switch_subs->getClickEvent()->subscribe([item_scan_switch_subs](brls::View* view) {
            if (get_setting(setting_search_subfolders) == "true")
            {
                set_setting(setting_search_subfolders, "false");
                item_scan_switch_subs->setChecked(false);
            }
            else
            {
                set_setting(setting_search_subfolders, "true");
                item_scan_switch_subs->setChecked(true);
            }
        });

        brls::ListItem* item_scan_root = new brls::ListItem("Scan / (not subfolders)");
        item_scan_root->setChecked((get_setting(setting_search_root) == "true"));
        item_scan_root->updateActionHint(brls::Key::A, "Toggle");
        item_scan_root->getClickEvent()->subscribe([item_scan_root](brls::View* view) {
            if (get_setting(setting_search_root) == "true")
            {
                set_setting(setting_search_root, "false");
                item_scan_root->setChecked(false);
            }
            else
            {
                set_setting(setting_search_root, "true");
                item_scan_root->setChecked(true);
            }
        });

        brls::SelectListItem* layerSelectItem = new brls::SelectListItem("Scan Range", { "Scan Whole SD Card (Slow!)", "Only scan some folders" });

        layerSelectItem->getValueSelectedEvent()->subscribe([item_scan_switch, item_scan_switch_subs, item_scan_root](size_t selection) {
            switch (selection)
            {
                case 1:
                    set_setting(setting_scan_full_card, "false");
                    item_scan_switch->expand(true);
                    item_scan_switch_subs->expand(true);
                    item_scan_root->expand(true);
                    break;
                case 0:
                    set_setting(setting_scan_full_card, "true");
                    item_scan_switch->collapse(true);
                    item_scan_switch_subs->collapse(true);
                    item_scan_root->collapse(true);
                    break;
            }
        });
        settings_list->addView(layerSelectItem);
        settings_list->addView(item_scan_switch);
        settings_list->addView(item_scan_switch_subs);
        settings_list->addView(item_scan_root);

        if (get_setting(setting_scan_full_card) == "false")
        {
            layerSelectItem->setSelectedValue(1);
            item_scan_switch->expand(true);
            item_scan_switch_subs->expand(true);
            item_scan_root->expand(true);
        }
        else
        {
            layerSelectItem->setSelectedValue(0);
            item_scan_switch->collapse(true);
            item_scan_switch_subs->collapse(true);
            item_scan_root->collapse(true);
        }

        //
        print_debug("Misc.\n");
        settings_list->addView(new brls::Header("Misc. Settings"));

        brls::ListItem* debug_switch = new brls::ListItem("Debug Mode", "Takes full effect on next launch.");
        debug_switch->setChecked((get_setting(setting_debug) == "true"));
        debug_switch->updateActionHint(brls::Key::A, "Toggle");
        debug_switch->getClickEvent()->subscribe([debug_switch](brls::View* view) {
            if (get_setting(setting_debug) == "true")
            {
                set_setting(setting_debug, "false");
                debug_switch->setChecked(false);
            }
            else
            {
                set_setting(setting_debug, "true");
                debug_switch->setChecked(true);
            }
        });
        settings_list->addView(debug_switch);
        //

        this->addTab("Settings", settings_list);
    }

    print_debug("Debug Menu.\n");
    if (get_setting_true(setting_debug))
    {
        this->addSeparator();
        brls::List* debug_list = new brls::List();
        debug_list->addView(new brls::Header("Super Secret Dev Menu Unlocked!", false));

        psmInitialize();

        std::uint32_t batteryCharge = 0;
        psmGetBatteryChargePercentage(&batteryCharge);
        add_list_entry("Battery Percent", std::to_string(batteryCharge) + "%", "", debug_list);

        ChargerType chargerType;
        std::string chargerTypes[3] = { std::string("None"), std::string("Charging"), std::string("USB") };
        psmGetChargerType(&chargerType);

        add_list_entry("Charging Status", chargerTypes[chargerType], "", debug_list);
        add_list_entry("Local Version", std::string("v") + get_setting(setting_local_version), "", debug_list);
        add_list_entry("Online Version", std::string("v") + get_online_version_number(), "", debug_list);
        add_list_entry("Number of App Store Apps", std::to_string(store_apps.size()), "", debug_list);
        add_list_entry("Number of Local Apps", std::to_string(local_apps.size()), "", debug_list);

        brls::ListItem* rtp_item = new brls::ListItem("Reboot to Payload");
        rtp_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("reboot_to_payload\n");
            reboot_to_payload();
        });
        debug_list->addView(rtp_item);

        brls::ListItem* launch_item = new brls::ListItem("Launch App");
        launch_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("launch app\n");
            //launch_nro("", "");
            brls::Application::quit();
        });
        debug_list->addView(launch_item);

        this->addTab("Debug Menu", debug_list);
    }

    print_debug("rm lock.\n");

    if (fs::exists("sdmc:/config/homebrew_details/lock"))
        remove("sdmc:/config/homebrew_details/lock");
}

MainPage::~MainPage()
{
}
