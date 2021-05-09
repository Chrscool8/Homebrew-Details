#include <utils/blacklist.h>
#include <utils/launching.h>
#include <utils/panels.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <borealis.hpp>
#include <pages/info_page.hpp>
#include <pages/intro_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

void show_update_panel()
{
    brls::TabFrame* appView = new brls::TabFrame();
    appView->sidebar->setWidth(1000);
    std::string vers = std::string("") + " v" + APP_VERSION + "  " + " " + symbol_rightarrow() + " " + "  v" + get_online_version_number() + "\n\n";
    appView->sidebar->addView(new brls::Header("Update Actions", false));
    brls::ListItem* dialogItem = new brls::ListItem("Update Wizard");

    dialogItem->getClickEvent()->subscribe([&](brls::View* view) {
        brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
        stagedFrame->setTitle("Update Wizard");
        stagedFrame->setIcon(get_resource_path() + "icon.png");
        stagedFrame->setActionAvailable(brls::Key::B, false);

        stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_intro));
        stagedFrame->addStage(new UpdatingPage(stagedFrame));
        stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_done));

        brls::Application::pushView(stagedFrame);
    });

    print_debug(get_online_version_number());

    appView->sidebar->addView(dialogItem);
    appView->sidebar->addView(new brls::Label(brls::LabelStyle::REGULAR, " \n ", true));
    appView->sidebar->addView(new brls::Header("New Version Details", false));
    appView->sidebar->addView(add_list_entry("Online Version", "v" + get_online_version_number(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Title", get_online_version_name(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Description", get_online_version_description(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Date", get_online_version_date(), "", NULL, 40));

    appView->setIcon(get_resource_path() + "download.png");
    brls::PopupFrame::open("Update Info", appView, vers, "");
}

void show_settings_panel()
{
    brls::TabFrame* appView = new brls::TabFrame();

    {
        print_debug("Settings.");

        brls::List* settings_list_scan = new brls::List();
        settings_list_scan->addView(new brls::Header("Scan Settings"));

        brls::ListItem* autoscan_switch = new brls::ListItem("Autoscan", "", "Begin scanning as soon as the app is launched.");
        autoscan_switch->setChecked((settings_get_value_true("scan", "autoscan")));
        autoscan_switch->updateActionHint(brls::Key::A, "Toggle");
        autoscan_switch->getClickEvent()->subscribe([autoscan_switch](brls::View* view) {
            if (settings_get_value("scan", "autoscan") == "true")
            {
                settings_set_value("scan", "autoscan", "false");
                autoscan_switch->setChecked(false);
            }
            else
            {
                settings_set_value("scan", "autoscan", "true");
                autoscan_switch->setChecked(true);
            }
        });
        settings_list_scan->addView(autoscan_switch);

        brls::ListItem* item_scan_switch = new brls::ListItem("Scan /switch/");
        item_scan_switch->setChecked(true);
        brls::ListItem* item_scan_switch_subs = new brls::ListItem("Scan /switch/'s subfolders");
        item_scan_switch_subs->setChecked(settings_get_value_true("scan", "subfolders"));
        item_scan_switch_subs->updateActionHint(brls::Key::A, "Toggle");
        item_scan_switch_subs->getClickEvent()->subscribe([item_scan_switch_subs](brls::View* view) {
            if (settings_get_value("scan", "subfolders") == "true")
            {
                settings_set_value("scan", "subfolders", "false");
                item_scan_switch_subs->setChecked(false);
            }
            else
            {
                settings_set_value("scan", "subfolders", "true");
                item_scan_switch_subs->setChecked(true);
            }

            settings_set_value("scan", "settings changed", "true");
        });

        brls::ListItem* item_scan_root = new brls::ListItem("Scan / (not subfolders)");
        item_scan_root->setChecked((settings_get_value_true("scan", "root")));
        item_scan_root->updateActionHint(brls::Key::A, "Toggle");
        item_scan_root->getClickEvent()->subscribe([item_scan_root](brls::View* view) {
            if (settings_get_value("scan", "root") == "true")
            {
                settings_set_value("scan", "root", "false");
                item_scan_root->setChecked(false);
            }
            else
            {
                settings_set_value("scan", "root", "true");
                item_scan_root->setChecked(true);
            }

            settings_set_value("scan", "settings changed", "true");
        });

        brls::SelectListItem* layerSelectItem = new brls::SelectListItem("Scan Range", { "Scan Whole SD Card (Slow!)", "Only scan some folders" });

        layerSelectItem->getValueSelectedEvent()->subscribe([item_scan_switch, item_scan_switch_subs, item_scan_root](size_t selection) {
            switch (selection)
            {
                case 1:
                    settings_set_value("scan", "full card", "false");
                    item_scan_switch->expand(true);
                    item_scan_switch_subs->expand(true);
                    item_scan_root->expand(true);
                    break;
                case 0:
                    settings_set_value("scan", "full card", "true");
                    item_scan_switch->collapse(true);
                    item_scan_switch_subs->collapse(true);
                    item_scan_root->collapse(true);
                    break;
            }

            settings_set_value("scan", "settings changed", "true");
        });
        settings_list_scan->addView(layerSelectItem);
        settings_list_scan->addView(item_scan_switch);
        settings_list_scan->addView(item_scan_switch_subs);
        settings_list_scan->addView(item_scan_root);

        if (settings_get_value("scan", "full card") == "false")
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

        appView->addTab("Scanning", settings_list_scan);
    }
    //
    {
        brls::List* settings_list_blacklist = new brls::List();
        settings_list_blacklist->addView(new brls::Header("Blacklist Settings"));

        std::vector<std::string> bl_vec;
        bl_vec.push_back("Add new entry...");
        for (unsigned int ii = 0; ii < blacklist.size(); ii++)
            bl_vec.push_back(blacklist.at(ii));

        brls::ListItem* bl_edit_item = new brls::ListItem("Edit Blacklist", "Blacklisted folders won't be scanned.");
        bl_edit_item->registerAction("Edit", brls::Key::A, [&]() {
            brls::TabFrame* appView = new brls::TabFrame();
            appView->sidebar->setWidth(1000);
            appView->addTab("Add new folder", nullptr);
            appView->sidebar->getChild(0)->registerAction("OK", brls::Key::A, [appView]() {
                std::string input = get_keyboard_input("sdmc:/switch/ignore_this");
                if (!input.empty())
                    add_blacklist(input);

                return appView->onCancel();
            });

            if (!blacklist.empty())
                appView->addSeparator();

            for (unsigned int iii = 0; iii < blacklist.size(); iii++)
            {
                const char* item_name = blacklist.at(iii).c_str();
                appView->addTab(item_name, nullptr);
                appView->sidebar->getChild(iii + 2)->registerAction("Delete Entry", brls::Key::X, [appView, item_name]() {
                    remove_blacklist(item_name);
                    return appView->onCancel();
                });
                appView->sidebar->getChild(iii + 2)->registerAction("Edit Entry", brls::Key::A, [appView, item_name]() {
                    std::string prev_string = item_name;
                    remove_blacklist(prev_string);

                    std::string input = get_keyboard_input(prev_string);
                    if (!input.empty())
                        add_blacklist(input);

                    return appView->onCancel();
                });
            }

            brls::PopupFrame::open("Blacklisted Folders", appView, "", "");
            return true;
        });
        settings_list_blacklist->addView(bl_edit_item);

        brls::Table* table = new brls::Table();
        if (blacklist.size() != 0)
            settings_list_blacklist->addView(new brls::Header("Blacklisted Folders"));
        for (unsigned int iii = 0; iii < blacklist.size(); iii++)
        {
            const char* item_name = blacklist.at(iii).c_str();
            table->addRow(brls::TableRowType::BODY, item_name);
        }

        settings_list_blacklist->addView(table);

        appView->addTab("Blacklist", settings_list_blacklist);
    }

    //
    {
        brls::List* settings_list_controls = new brls::List();
        settings_list_controls->addView(new brls::Header("Control Settings"));

        brls::SelectListItem* controlSelectItem = new brls::SelectListItem("Control Settings", { "A: Details; X: Launch", "A: Launch; X: Details" }, std::stoi(settings_get_value("preferences", "control scheme")), "Takes full effect on next launch.");
        controlSelectItem->getValueSelectedEvent()->subscribe([](size_t selection) {
            settings_set_value("preferences", "control scheme", std::to_string(selection));
        });
        settings_list_controls->addView(controlSelectItem);

        appView->addTab("Controls", settings_list_controls);
    }
    //
    appView->addSeparator();

    {
        print_debug("Toolbox.");
        brls::List* tools_list = new brls::List();

        tools_list->addView(new brls::Header("Actionables"));

        brls::ListItem* rta_item = new brls::ListItem("Restart App");
        rta_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("restart app");
            launch_nro(settings_get_value("meta", "nro path"), "\"" + settings_get_value("meta", "nro path") + "\"");
#ifdef __SWITCH__
            romfsExit();
#endif
            brls::Application::quit();
        });
        tools_list->addView(rta_item);

        brls::ListItem* rtp_item = new brls::ListItem("Reboot to Payload", "sdmc:/atmosphere/reboot_payload.bin");
        rtp_item->getClickEvent()->subscribe([](brls::View* view) {
            print_debug("reboot_to_payload");
            int result = reboot_to_payload();
            if (result == -1)
                brls::Application::notify("Problem initializing spl");
            else if (result == -2)
                brls::Application::notify("Failed to open atmosphere/ reboot_payload.bin!");
        });
        tools_list->addView(rtp_item);

        tools_list->addView(new brls::Header("Information"));

        brls::ListItem* nsp_item = new brls::ListItem("How to Install to Home Menu...", "Will take a few seconds to copy file on first click.");
        nsp_item->getClickEvent()->subscribe([](brls::View* view) {
            export_resource("forwarder", "HomebrewDetailsForwarder_v2.nsp");
            brls::TabFrame* appView = new brls::TabFrame();
            appView->sidebar->setWidth(1920);
            appView->setWidth(1920);
            appView->setHeight(400);
            appView->sidebar->addView(new brls::Label(brls::LabelStyle::REGULAR, "\nUsing your favorite nsp installer, install the forwarder that is currently in:\n\n" + get_config_path() + "forwarder/HomebrewDetailsForwarder_v2.nsp\n\nIt will launch this application from any of the following locations:\n- sdmc:/switch/homebrew_details.nro\n- sdmc:/switch/homebrew_details/homebrew_details.nro\n- sdmc:/switch/homebrew-details/homebrew_details.nro\n- sdmc:/switch/homebrew_details_alternate.nro", true));
            appView->setIcon(get_resource_path() + "arrows.png");
            brls::PopupFrame::open("How to Install to Home Menu", appView, "", "");
        });
        tools_list->addView(nsp_item);

        appView->addTab("Toolbox", tools_list);

        print_debug("Misc.");

        brls::List* settings_list_app = new brls::List();
        settings_list_app->addView(new brls::Header("App Settings"));

        brls::SelectListItem* exitToItem = new brls::SelectListItem("Exit To", { "sdmc:/hbmenu.nro", settings_get_value("meta", "nro path") });
        exitToItem->setValue(settings_get_value("meta", "exit to"));
        exitToItem->getValueSelectedEvent()->subscribe([](size_t selection) {
            if (selection == 0)
                settings_set_value("meta", "exit to", "sdmc:/hbmenu.nro");
            else if (selection == 1)
                settings_set_value("meta", "exit to", settings_get_value("meta", "nro path"));

            std::string target = settings_get_value("meta", "exit to");
#ifdef __SWITCH__
            envSetNextLoad(target.c_str(), (std::string("\"") + target + "\"").c_str());
#endif
        });
        settings_list_app->addView(exitToItem);

        settings_list_app->addView(new brls::Header("Forwarder Settings"));
        brls::ListItem* item = new brls::ListItem("Reset Automatic Forwarding");
        item->getClickEvent()->subscribe([item](brls::View* view) {
            settings_set_value("forwarder", "option", "Manually Choose");
            item->setChecked(true);
            return true;
        });
        settings_list_app->addView(item);

        settings_list_app->addView(new brls::Header("Misc. Settings"));

        brls::ListItem* debug_switch = new brls::ListItem("Debug Mode", "Takes full effect on next launch.");
        debug_switch->setChecked(settings_get_value_true("meta", "debug"));
        debug_switch->updateActionHint(brls::Key::A, "Toggle");
        debug_switch->getClickEvent()->subscribe([debug_switch](brls::View* view) {
            if (settings_get_value_true("meta", "debug"))
            {
                settings_set_value("meta", "debug", "false");
                debug_switch->setChecked(false);
            }
            else
            {
                settings_set_value("meta", "debug", "true");
                debug_switch->setChecked(true);
            }
        });
        settings_list_app->addView(debug_switch);
        appView->sidebar->setWidth(320);
        appView->addTab("Misc", settings_list_app);
    }
    //

    appView->setIcon(get_resource_path() + "download.png");
    brls::PopupFrame::open("Options", appView, "", "");
}

void show_first_time_panel()
{
    brls::TabFrame* appView = new brls::TabFrame();
    appView->sidebar->setWidth(385);

    brls::Image* credits = new brls::Image(get_resource_path() + "credits.png");
    credits->setScaleType(brls::ImageScaleType::NO_RESIZE);
    appView->addTab("Introduction", credits);

    appView->addSeparator();

    brls::List* list_about = new brls::List();
    list_about->addView(new brls::Header("What is it?"));
    list_about->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "This is an app that allows you to view details about, launch, categorize, and manage all the .nro files on your Switch using borealis for a native-feeling UI. It also includes a toolbox of handy quick actions like rebooting to a payload. It is nearly a feature-complete replacement/alternative to hbmenu.", true));

    list_about->addView(new brls::Header("How can you help?"));
    list_about->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "Comments, criticism, and suggestions are welcomed and encouraged. I'd love to make the greatest product I can for the community and I'm generally happy to cater to your specific requests when possible. All I ask of you is to enjoy using it. I'm making this in my free time, and if you'd like to monetarily support me to allow me to have more free time, you can find a sponsor/donation link on the GitHub page to toss me a buck, but this project will always be free for all.", true));

    list_about->setSpacing(20);

    appView->addTab("About this App", list_about);

    brls::List* list_features1 = new brls::List();
    list_features1->addView(new brls::Header("NRO Features"));
    list_features1->addView(new brls::Label(brls::LabelStyle::DESCRIPTION,
        "" + symbol_bullet() + "  Launch Apps\n" + ""
            + symbol_bullet() + "  View details about your apps such as:\n" + ""
            + " \u2015" + " " + "  Name, Filename, File Location, Author, Size\n" + ""
            + "" + symbol_bullet() + "  App Store Info including:\n" + ""
            + " \u2015" + " " + "  URL, Category, License, Description, Summary, Changelog\n" + ""
            + symbol_bullet() + "  Add custom notes to an app\n" + ""
            + symbol_bullet() + "  Rename, Copy, Move, Delete an app\n" + ""
            + symbol_bullet() + "  Pin/Favorite apps to the top of the list",
        true));

    appView->addTab("Features, Part 1", list_features1);

    brls::List* list_features2 = new brls::List();
    list_features2->addView(new brls::Header("Scanning Features"));
    list_features2->addView(new brls::Label(brls::LabelStyle::DESCRIPTION,
        "" + symbol_bullet() + "  Scan your SD card for homebrew programs (.nros)\n" + ""
            + symbol_bullet() + "  App Sorting and grouping\n" + ""
            + symbol_bullet() + "  Blacklist folders from search\n" + ""
            + symbol_bullet() + "  App list caching",
        true));

    list_features2->addView(new brls::Header("Toolbox Features"));
    list_features2->addView(new brls::Label(brls::LabelStyle::DESCRIPTION,
        "" + symbol_bullet() + "  Reboot to a payload\n" + symbol_bullet() + "  Restart the app\n" + symbol_bullet() + "  Installing to the home screen ",
        true));

    appView->addTab("Features, Part 2", list_features2);
    appView->addSeparator();

    brls::List* contact_list = new brls::List();
    contact_list->setSpacing(contact_list->getSpacing() / 2);

    contact_list->addView(new brls::Header("Chris Bradel"));
    contact_list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "Email: chrsdev8@gmail.com\n"
                                                                         "Discord: Chrscool8#0001\n"
                                                                         "Github: https://github.com/Chrscool8\n"
                                                                         "YouTube: https://www.youtube.com/chrscool8\n"
                                                                         "Twitch: https://www.twitch.tv/chrisbradel\n",
        true));

    contact_list->addView(new brls::Header("Let me know!"));
    contact_list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "Feel free to message me any time with questions, concerns, issues, or feature or app suggestions. You can also join the discussion on the following websites:", true));

    contact_list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "Github: https://github.com/Chrscool8/Homebrew-Details"));
    contact_list->addView(new brls::Label(brls::LabelStyle::DESCRIPTION, "GBATEMP Forum (Shortened Link): https://rebrand.ly/HBDetails"));

    appView->addTab("Contact", contact_list);

    appView->sidebar->registerAction("", brls::Key::DRIGHT, []() { return true; });

    appView->setIcon(get_resource_path() + "icon.png");
    appView->updateActionHint(brls::Key::B, "Back to Main Screen");
    brls::PopupFrame::open("Welcome To Homebrew Details", appView, "First Time Overview", "");
}

brls::List* generate_changelog_panel(std::string title, std::string str)
{
    brls::List* list = new brls::List();

    brls::Label* title_label = new brls::Label(brls::LabelStyle::REGULAR, title);
    title_label->setFontSize(30);
    list->addView(title_label);

    std::vector<std::string> lines = explode(str, '*');
    for (unsigned int i = 0; i < lines.size(); i++)
    {
        unsigned int width = 44;

        if (lines.at(i).size() != 0)
        {
            std::vector<std::string> words = explode(lines.at(i), ' ');

            std::string line = "";

            while (words.size() > 0)
            {
                line = "";
                while (line.size() < width && words.size() > 0)
                {
                    line += words.at(0) + " ";
                    words.erase(words.begin());
                }

                brls::ListItem* skinny2 = new brls::ListItem(line);

                skinny2->setHeight(35);
                skinny2->setDrawTopSeparator(false);
                skinny2->setDrawBottomSeparator(false);
                list->addView(skinny2);
            }
        }
        else
        {
            brls::ListItem* skinny2 = new brls::ListItem("");
            skinny2->setHeight(35);
            skinny2->setDrawTopSeparator(false);
            skinny2->setDrawBottomSeparator(false);
            list->addView(skinny2);
        }
    }

    if (list->getViewsCount() == 0)
    {
        brls::ListItem* skinny = new brls::ListItem("");
        skinny->setHeight(20);
        skinny->setForceTranslucent(true);
        list->addView(skinny);
    }

    return list;
}

void show_whatsnew_panel()
{
    brls::TabFrame* appView = new brls::TabFrame();
    appView->setIcon(get_resource_path() + "icon.png");
    appView->sidebar->setWidth(250);

    appView->addTab("v1.04", generate_changelog_panel("Long Time, No See", "Feature:*- Show app icon in app panel**Tweaks:*- Don't check for updates on first run*- Avoid potential slooooow black screen opening"));
    appView->addTab("v1.03", generate_changelog_panel("Encoding Fixes", "Feature:*- Info module on main screen**Tweaks:*- Battery info notice when info not available**Crash Fixes:*- Check text encoding before writing to json*- Stricter validation of nacp info to avoid crashing or reading garbage"));
    appView->addTab("v1.01", generate_changelog_panel("New Forwarder App", "Features:**- New \"What's New\" panel on main screen that shows recent changelogs (like this!)*- New Forwarder .nsp installable to your home menu**Fixes:**- Tiny potential problem affecting early 1.0 updates fixed*- Fix situation where blacklists may not apply**New Forwarder Overview:**- Display versions of HB-D on your card*- Choose and run your choice manually*- Automatically run your choice without additional input*- Included in this update and can be found at \"sdmc:/config/homebrew_details_next/forwarder*/HomebrewDetailsForwarder_v2.nsp\""));
    appView->addTab("v1.0 ", generate_changelog_panel("The Complete Overhaul", "Long time, no see! For the last four months or so I've been rewriting the application almost completely from the ground up. Here's what's new.**Features:**- Scan files more than twice as fast*- Cache app lists to json and instantly reload them on next run without having to rescan*- Cache app icons for instantly reloading menus*- Sort list by categories (name, path, author, size, category)*- Sort list ascending or descending*- Group sorted lists by category (none, author, category, from appstore)*- Groundwork for language localization*- Groundwork for multiple view styles (icon grid and icon list coming soon)*- Welcome Page and Introduction for first run*- Settings is now a categorized json*- Pin Favorite apps to the top of the list*- Move Settings and Updater into their own panels accessible by list or main screen*- Delete folder if deleting app and it's the only thing in the folder*- Basically every other aspect rewritten for speed and stability**Frameworks:**- Updated nlohmann json*- Updated libnx*- Updated borealis ui*- Updated curl"));
    appView->addTab("v0.95", generate_changelog_panel("Exit-To Settings", "- Added a new setting of what to exit to (between hb-menu and hb-d)"));
    appView->addTab("v0.94", generate_changelog_panel("New Multi-Forwarder", "- Added a new multi-forwarder nsp that you can install to your home menu so you can organize the actual hb-d.nro where you'd like also an info panel with this information in the toolbox tab"));
    appView->addTab("v0.93", generate_changelog_panel("Favorite App Pinning, Blacklist Folders", "- Added a new panel in the settings menu where you can add blacklisted folders (they'll be skipped during search)*- Added the ability to pin your favorite apps to the top of the list for easy access"));
    appView->addTab("v0.92", generate_changelog_panel("Search Folder Blacklister", "- Added a new panel in the settings menu where you can add blacklisted folders (they'll be skipped during search)*- [Debug Mode Feature] Pinning favorite apps is mostly working but not quite done. If you wanna try it, you can enable debug mode in the settings!"));

    brls::PopupFrame::open("What's New?", appView, "Recent Changelogs", "");
}