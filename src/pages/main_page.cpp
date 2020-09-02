#include <main.h>
#include <utils/favorites.h>
#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/notes.h>
#include <utils/reboot_to_payload.h>
#include <utils/scanning.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <pages/info_page.hpp>
#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>
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
#include <time.h>

#include <algorithm>
#include <array>
#include <borealis.hpp>
#include <cassert>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

namespace fs = std::filesystem;

brls::ListItem* MainPage::add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length = 21)
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

    add_to->addView(item);
    return item;
}

void purge_entry(app_entry* entry)
{
}

brls::ListItem* MainPage::make_app_entry(app_entry* entry, bool is_appstore)
{
    std::string label = entry->name;

    brls::ListItem* popupItem = new brls::ListItem(label, "", entry->full_path);
    popupItem->setValue("v" + entry->version);
    popupItem->setThumbnail(entry->icon, entry->icon_size);

    if (get_setting_true(setting_debug))
    {
        popupItem->updateActionHint(brls::Key::Y, "Favorite");
        popupItem->registerAction("Favorite", brls::Key::Y, [this, entry, popupItem]() {
            if (vector_contains(favorites, entry->full_path))
            {
                remove_favorite(entry->full_path);
                entry->favorite = false;

                print_debug("check: " + entry->name + " " + "v" + entry->version);
                print_debug(std::to_string(appsList->getViewsCount()));

                for (unsigned int i = 0; i < appsList->getViewsCount(); i++)
                {
                    print_debug(std::to_string(i));
                    brls::ListItem* item = (brls::ListItem*)appsList->getChild(i);
                    //print_debug("item+: " + item->getLabel() + ";" + item->getValue());
                    //print_debug("item*: " + symbol_star() + "  " + entry->name + ";" + "v" + entry->version);

                    if (symbol_star() + "  " + entry->name == item->getLabel() && "v" + entry->version == item->getValue())
                    {
                        print_debug("Collapse: " + item->getLabel() + " " + item->getValue());

                        item->collapse(false);
                    }
                    else if (entry->name == item->getLabel() && "v" + entry->version == item->getValue())
                    {
                        print_debug("Expand: " + item->getLabel() + " " + item->getValue());

                        item->expand(false);
                    }
                }

                //popupItem->setChecked(false);
            }
            else
            {
                add_favorite(entry->full_path);
                entry->favorite = true;

                print_debug("check: " + entry->name + " " + "v" + entry->version);
                print_debug(std::to_string(appsList->getViewsCount()));

                for (unsigned int i = 0; i < appsList->getViewsCount(); i++)
                {
                    print_debug(std::to_string(i));

                    brls::ListItem* item = (brls::ListItem*)appsList->getChild(i);
                    //print_debug("item+: " + item->getLabel() + ";" + item->getValue());
                    //print_debug("item*: " + symbol_star() + "  " + entry->name + ";" + "v" + entry->version);

                    if (symbol_star() + "  " + entry->name == item->getLabel() && "v" + entry->version == item->getValue())
                    {
                        print_debug("Collapse: " + item->getLabel() + " " + item->getValue());

                        item->expand(false);
                    }
                    else if (entry->name == item->getLabel() && "v" + entry->version == item->getValue())
                    {
                        print_debug("Expand: " + item->getLabel() + " " + item->getValue());

                        item->collapse(false);
                    }
                }
            }

            appsList->invalidate();

            return true;
        });
    }

    brls::Key key = brls::Key::A;
    if (get_setting(setting_control_scheme) == "0")
        key = brls::Key::X;
    else if (get_setting(setting_control_scheme) == "1")
        key = brls::Key::A;

    popupItem->updateActionHint(key, "Launch");
    popupItem->registerAction("Launch", key, [this, entry, popupItem]() {
        print_debug("launch app");
        unsigned int r = launch_nro(entry->full_path, "\"" + entry->full_path + "\"");
        print_debug("r: " + std::to_string(r));
        if (R_FAILED(r))
        {
            print_debug("Uh oh.");
        }
        else
        {
            local_apps.clear();
            store_apps.clear();
            store_file_data.clear();
            romfsExit();
            brls::Application::quit();
        }

        return true;
    });

    key = brls::Key::X;
    if (get_setting(setting_control_scheme) == "0")
        key = brls::Key::A;
    else if (get_setting(setting_control_scheme) == "1")
        key = brls::Key::X;

    popupItem->updateActionHint(key, "Details");
    popupItem->registerAction("Details", key, [this, entry, popupItem, is_appstore]() {
        brls::TabFrame* appView = new brls::TabFrame();

        brls::List* manageList = new brls::List();
        manageList->addView(new brls::Header("File Management Actions", false));

        {
            brls::ListItem* launch_item = new brls::ListItem("Launch App");
            launch_item->getClickEvent()->subscribe([this, entry](brls::View* view) {
                print_debug("launch app");
                unsigned int r = launch_nro(entry->full_path, "\"" + entry->full_path + "\"");
                print_debug("r: " + std::to_string(r));
                if (R_FAILED(r))
                {
                    print_debug("Uh oh.");
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

        //
        if (!is_appstore)
        {
            brls::ListItem* move_item = new brls::ListItem("Move App");
            move_item->getClickEvent()->subscribe([entry, appView](brls::View* view) {
                std::string dest_string = get_keyboard_input(entry->full_path);

                if (to_lower(dest_string) == to_lower(entry->full_path))
                {
                    print_debug("Same path");
                    brls::Dialog* dialog                       = new brls::Dialog("Source and destination are the same.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else if (dest_string.length() <= 4 || (dest_string).substr(dest_string.length() - 4) != ".nro")
                {
                    print_debug("Isn't an nro");
                    brls::Dialog* dialog                       = new brls::Dialog("Your destination,\n'" + dest_string + "'\ndoesn't end with '.nro'.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else if (fs::exists(dest_string))
                {
                    print_debug("File already exists.");
                    brls::Dialog* dialog                       = new brls::Dialog("Your destination,\n'" + dest_string + "'\nalready exists.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else
                {
                    std::size_t found = dest_string.find_last_of("/");
                    if (found != std::string::npos)
                    {
                        create_directories(dest_string.substr(0, found));
                    }

                    brls::Dialog* confirm_dialog             = new brls::Dialog("Are you sure you want to move the following file? This action cannot be undone.\n\n" + entry->full_path + "\n" + symbol_downarrow() + "\n" + dest_string);
                    brls::GenericEvent::Callback yesCallback = [confirm_dialog, entry, appView, dest_string](brls::View* view) {
                        if (rename(entry->full_path.c_str(), dest_string.c_str()) != 0)
                            brls::Application::notify("Issue moving file");
                        else
                        {
                            brls::Application::notify("File successfully moved");
                            purge_entry(entry);
                        }

                        confirm_dialog->close();
                    };
                    brls::GenericEvent::Callback noCallback = [confirm_dialog](brls::View* view) {
                        confirm_dialog->close();
                    };
                    confirm_dialog->addButton("!!  [Yes]  !!", yesCallback);
                    confirm_dialog->addButton("No", noCallback);
                    confirm_dialog->setCancelable(false);
                    confirm_dialog->open();
                }
            });
            manageList->addView(move_item);
        }
        //
        {
            brls::ListItem* copy_item = new brls::ListItem("Copy App");
            copy_item->getClickEvent()->subscribe([entry, appView](brls::View* view) {
                std::string dest_string = get_keyboard_input(entry->full_path);

                if (to_lower(dest_string) == to_lower(entry->full_path))
                {
                    print_debug("Same path");
                    brls::Dialog* dialog                       = new brls::Dialog("Source and destination are the same.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else if (dest_string.length() <= 4 || (dest_string).substr(dest_string.length() - 4) != ".nro")
                {
                    print_debug("Isn't an nro");
                    brls::Dialog* dialog                       = new brls::Dialog("Your destination,\n'" + dest_string + "'\ndoesn't end with '.nro'.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else if (fs::exists(dest_string))
                {
                    print_debug("File already exists.");
                    brls::Dialog* dialog                       = new brls::Dialog("Your destination,\n'" + dest_string + "'\nalready exists.");
                    brls::GenericEvent::Callback closeCallback = [dialog](brls::View* view) {
                        dialog->close();
                    };
                    dialog->addButton("Dismiss", closeCallback);
                    dialog->setCancelable(true);
                    dialog->open();
                }
                else
                {
                    std::size_t found = dest_string.find_last_of("/");
                    if (found != std::string::npos)
                    {
                        create_directories(dest_string.substr(0, found));
                    }

                    brls::Dialog* confirm_dialog             = new brls::Dialog("Are you sure you want to copy to the following file? This action cannot be undone.\n\n" + entry->full_path + "\n" + symbol_downarrow() + "\n" + dest_string);
                    brls::GenericEvent::Callback yesCallback = [confirm_dialog, entry, appView, dest_string](brls::View* view) {
                        if (copy_file(entry->full_path.c_str(), dest_string.c_str()))
                        {
                            brls::Application::notify("File successfully copied");
                            purge_entry(entry);
                        }
                        else
                        {
                            brls::Application::notify("Issue copying file");
                        }

                        confirm_dialog->close();
                    };
                    brls::GenericEvent::Callback noCallback = [confirm_dialog](brls::View* view) {
                        confirm_dialog->close();
                    };
                    confirm_dialog->addButton("!!  [Yes]  !!", yesCallback);
                    confirm_dialog->addButton("No", noCallback);
                    confirm_dialog->setCancelable(false);
                    confirm_dialog->open();
                }
            });
            manageList->addView(copy_item);
        }
        //
        {
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
            manageList->addView(delete_item);
        }

        appView->addTab("Manage", manageList);

        brls::List* appInfoList = new brls::List();
        appInfoList->addView(new brls::Header(".NRO File Info", false));
        add_list_entry("Name", entry->name, "", appInfoList);
        add_list_entry("Filename", entry->file_name, "Full Path:\n\n" + entry->full_path, appInfoList);
        add_list_entry("Author", entry->author, "", appInfoList);
        add_list_entry("Version", entry->version, "", appInfoList);
        add_list_entry("Size", to_megabytes(entry->size) + " MB", "Exact Size:\n\n" + std::to_string(entry->size) + " bytes", appInfoList);
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

        {
            brls::List* notesList      = new brls::List();
            brls::ListItem* notes_item = new brls::ListItem("Edit Notes");
            brls::Label* desc          = new brls::Label(brls::LabelStyle::DESCRIPTION, notes_get_value(entry->file_name), true);
            brls::Label* note_header   = new brls::Label(brls::LabelStyle::REGULAR, "Notes:", false);
            note_header->setVerticalAlign(NVGalign::NVG_ALIGN_TOP);

            if (notes_get_value(entry->file_name).empty())
                note_header->collapse(true);
            else
                note_header->expand(true);

            notes_item->getClickEvent()->subscribe([entry, appView, desc, note_header](brls::View* view) {
                std::string dest_string = get_keyboard_input(notes_get_value(entry->file_name));
                notes_set_value(entry->file_name, dest_string);
                desc->setText(dest_string);

                if (dest_string.empty())
                    note_header->collapse(true);
                else
                    note_header->expand(true);
            });

            notesList->addView(notes_item);
            notesList->addView(note_header);
            notesList->addView(desc);
            appView->addTab("Notes", notesList);
        }

        brls::PopupFrame::open(entry->name, entry->icon, entry->icon_size, appView, "Author: " + entry->author, "Version: " + entry->version);

        return true;
    });

    return popupItem;
}

std::vector<brls::ListItem*> to_collapse;

void MainPage::build_main_tabs()
{
    appsList      = new brls::List();
    storeAppsList = new brls::List();
    localAppsList = new brls::List();

    for (unsigned int i = 0; i < local_apps.size(); i++)
    {
        app_entry* current  = &local_apps.at(i);
        std::string old_str = current->name;
        current->name       = symbol_star() + "  " + current->name;

        brls::ListItem* item_apps = make_app_entry(current, false);

        //if (!current->favorite)
        //    item_apps->collapse();

        if (!current->favorite)
            to_collapse.push_back(item_apps);

        appsList->addView(item_apps);

        if (current->from_appstore)
        {
            brls::ListItem* item_store = make_app_entry(current, true);
            storeAppsList->addView(item_store);
        }
        else
        {
            brls::ListItem* item_local = make_app_entry(current, false);
            localAppsList->addView(item_local);
        }

        current->name = old_str;
    }

    for (unsigned int i = 0; i < local_apps.size(); i++)
    {
        app_entry* current = &local_apps.at(i);

        brls::ListItem* item_apps = make_app_entry(current, false);

        //if (current->favorite)
        //    item_apps->collapse();

        if (current->favorite)
            to_collapse.push_back(item_apps);

        appsList->addView(item_apps);

        if (current->from_appstore)
        {
            brls::ListItem* item_store = make_app_entry(current, true);
            storeAppsList->addView(item_store);
        }
        else
        {
            brls::ListItem* item_local = make_app_entry(current, false);
            localAppsList->addView(item_local);
        }
    }

    if (!local_apps.empty() && !store_apps.empty())
    {
        this->addTab(pad_string_with_spaces("All Apps", store_apps.size() + local_apps.size(), 20).c_str(), appsList);
        this->addSeparator();
    }
    if (!store_apps.empty())
        this->addTab(pad_string_with_spaces("App Store Apps", store_apps.size(), 9).c_str(), storeAppsList);
    if (!local_apps.empty())
        this->addTab(pad_string_with_spaces("Local Apps", local_apps.size(), 16).c_str(), localAppsList);
}

void hide_by_fav(brls::List* list)
{
    while (!to_collapse.empty())
    {
        to_collapse.at(0)->collapse(false);
        to_collapse.erase(to_collapse.begin());
    }
}

MainPage::MainPage()
{
    std::string title = "Homebrew Details v" + get_setting(setting_local_version);
    if (get_setting_true(setting_debug))
        title += " [Debug Mode]";

    this->setTitle(title.c_str());
    this->setIcon(get_resource_path("icon.jpg"));
    print_debug("init rootframe");

    build_main_tabs();

    hide_by_fav(appsList);

    //rootFrame->addSeparator();
    //rootFrame->addTab("Applications", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Emulators", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Games", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Tools", new brls::Rectangle(nvgRGB(120, 120, 120)));
    //rootFrame->addTab("Misc.", new brls::Rectangle(nvgRGB(120, 120, 120)));

    //this->addTab("Read: "+std::to_string(batteryCharge), new brls::Rectangle(nvgRGB(120, 120, 120)));

    print_debug("Check for updates.");
    if (get_online_version_available())
    {
        this->addSeparator();
        brls::List* settingsList = new brls::List();
        settingsList->addView(new brls::Header("Update Actions", false));

        brls::ListItem* dialogItem = new brls::ListItem("Update Wizard", "v" + get_setting(setting_local_version) + "  " + " " + symbol_rightarrow() + " " + "  v" + get_online_version_number());
        dialogItem->getClickEvent()->subscribe([this](brls::View* view) {
            brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
            stagedFrame->setTitle("Update Wizard");
            stagedFrame->setIcon(get_resource_path("icon.jpg"));
            stagedFrame->setActionAvailable(brls::Key::B, false);
            //stagedFrame->updateActionHint(brls::Key::B, "");

            stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_intro));
            stagedFrame->addStage(new UpdatingPage(stagedFrame));
            stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_done));

            brls::Application::pushView(stagedFrame);
        });
        settingsList->addView(dialogItem);

        settingsList->addView(new brls::Header("New Version Details", false));
        add_list_entry("Online Version", "v" + get_online_version_number(), "", settingsList, 40);
        add_list_entry("Title", get_online_version_name(), "", settingsList, 40);
        add_list_entry("Description", get_online_version_description(), "", settingsList, 40);
        add_list_entry("Date", get_online_version_date(), "", settingsList, 40);

        this->addTab("Update Available!", settingsList);
    }

    print_debug("Toolbox.");
    {
        this->addSeparator();

        brls::List* tools_list   = new brls::List();
        brls::ListItem* rtp_item = new brls::ListItem("Reboot to Payload");
        rtp_item->setValue("atmosphere/reboot_payload.bin");
        rtp_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("reboot_to_payload");
            int result = reboot_to_payload();
            if (result == -1)
                brls::Application::notify("Problem initializing spl");
            else if (result == -2)
                brls::Application::notify("Failed to open atmosphere/ reboot_payload.bin!");
        });
        tools_list->addView(rtp_item);
        this->addTab("Toolbox", tools_list);
    }

    print_debug("Settings.");
    {
        brls::List* settings_list = new brls::List();
        settings_list->addView(new brls::Header("Scan Settings"));

        brls::ListItem* autoscan_switch = new brls::ListItem("Autoscan", "", "Begin scanning as soon as the app is launched.");
        autoscan_switch->setChecked((get_setting_true(setting_autoscan)));
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
        item_scan_switch_subs->setChecked((get_setting_true(setting_search_subfolders)));
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

            set_setting(setting_scan_settings_changed, "true");
        });

        brls::ListItem* item_scan_root = new brls::ListItem("Scan / (not subfolders)");
        item_scan_root->setChecked((get_setting_true(setting_search_root)));
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

            set_setting(setting_scan_settings_changed, "true");
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

            set_setting(setting_scan_settings_changed, "true");
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
        settings_list->addView(new brls::Header("Control Settings"));

        brls::SelectListItem* controlSelectItem = new brls::SelectListItem("Control Settings", { "A: Details; X: Launch", "A: Launch; X: Details" }, std::stoi(get_setting(setting_control_scheme)), "Takes full effect on next launch.");
        controlSelectItem->getValueSelectedEvent()->subscribe([](size_t selection) {
            set_setting(setting_control_scheme, std::to_string(selection));
        });
        settings_list->addView(controlSelectItem);

        //
        print_debug("Misc.");
        settings_list->addView(new brls::Header("Misc. Settings"));

        brls::ListItem* debug_switch = new brls::ListItem("Debug Mode", "Takes full effect on next launch.");
        debug_switch->setChecked(get_setting_true(setting_debug));
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

    print_debug("Debug Menu.");
    if (get_setting_true(setting_debug))
    {
        this->addSeparator();
        brls::List* debug_list = new brls::List();
        debug_list->addView(new brls::Header("Super Secret Dev Menu Unlocked!", false));

        std::uint32_t batteryCharge = 0;
        psmGetBatteryChargePercentage(&batteryCharge);
        add_list_entry("Battery Percent", std::to_string(batteryCharge) + "%", "", debug_list);

        ChargerType chargerType;
        std::string chargerTypes[3] = { std::string("None"), std::string("Charging"), std::string("USB") };
        psmGetChargerType(&chargerType);
        std::string chargeStatus = "Error";
        if ((int)chargerType >= 0 && (int)chargerType < 3)
            chargeStatus = chargerTypes[chargerType];

        add_list_entry("Charging Status", chargeStatus, "", debug_list);
        add_list_entry("Local Version", std::string("v") + get_setting(setting_local_version), "", debug_list);
        add_list_entry("Online Version", std::string("v") + get_online_version_number(), "", debug_list);
        add_list_entry("Number of App Store Apps", std::to_string(store_apps.size()), "", debug_list);
        add_list_entry("Number of Local Apps", std::to_string(local_apps.size()), "", debug_list);
        add_list_entry("Free Space", get_free_space(), "", debug_list);

        brls::ListItem* rtp_item = new brls::ListItem("Reboot to Payload");
        rtp_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("reboot_to_payload");
            reboot_to_payload();
        });
        debug_list->addView(rtp_item);

        brls::ListItem* keyboard_item = new brls::ListItem("Keyboard Test");
        keyboard_item->setValue("Test Text");
        keyboard_item->getClickEvent()->subscribe([keyboard_item](brls::View* view) {
            print_debug("Keyboard");
            std::string typed = get_keyboard_input(keyboard_item->getValue());
            keyboard_item->setValue(typed);
        });
        debug_list->addView(keyboard_item);

        this->addTab("Debug Menu", debug_list);
    }

    print_debug("rm lock.");

    if (fs::exists("sdmc:/config/homebrew_details/lock"))
        remove("sdmc:/config/homebrew_details/lock");

    set_setting(setting_scan_settings_changed, "false");

    ///////////////////////

    this->battery_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    this->battery_label->setHorizontalAlign(NVG_ALIGN_RIGHT);
    this->battery_label->setParent(this);

    this->time_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    this->time_label->setHorizontalAlign(NVG_ALIGN_LEFT);
    this->time_label->setParent(this);

    this->date_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    this->date_label->setHorizontalAlign(NVG_ALIGN_LEFT);
    this->date_label->setParent(this);
}

MainPage::~MainPage()
{
    delete time_label;
    delete battery_label;
    delete date_label;
}

void MainPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    TabFrame::draw(vg, x, y, width, height, style, ctx);

    this->battery_label->setFontSize(18);
    this->battery_label->setText("Battery" + get_battery_status() + ": " + std::to_string(get_battery_percent()) + "%");
    this->battery_label->setBoundaries(x + this->width - this->battery_label->getWidth() - 50, y + style->AppletFrame.headerHeightRegular * .5 + 14 + 4, this->battery_label->getWidth(), this->battery_label->getHeight());
    this->battery_label->invalidate(true);
    this->battery_label->frame(ctx);

    this->time_label->setFontSize(18);
    this->time_label->setText(get_time());
    this->time_label->setBoundaries(x + this->width - this->time_label->getWidth() - 50, y + style->AppletFrame.headerHeightRegular * .5 - 14 + 4, this->time_label->getWidth(), this->time_label->getHeight());
    this->time_label->invalidate(true);
    this->time_label->frame(ctx);

    this->date_label->setFontSize(18);
    this->date_label->setText(get_date() + "   |");
    this->date_label->setBoundaries(x + this->width - this->date_label->getWidth() - 145, y + style->AppletFrame.headerHeightRegular * .5 - 14 + 4, this->date_label->getWidth(), this->date_label->getHeight());
    this->date_label->invalidate(true);
    this->date_label->frame(ctx);
}