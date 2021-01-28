#include <utils/blacklist.h>
#include <utils/panels.h>
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
    std::string vers = " v" + settings_get_value("meta", "local version") + "  " + " " + symbol_rightarrow() + " " + "  v" + get_online_version_number() + "\n\n";
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

        brls::ListItem* bl_edit_item = new brls::ListItem("Edit Blacklist");
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
        //table->addRow(brls::TableRowType::HEADER, "Blacklisted Folders");
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

    {
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
            envSetNextLoad(target.c_str(), (std::string("\"") + target + "\"").c_str());
        });
        settings_list_app->addView(exitToItem);

        print_debug("Misc.");
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
        appView->addTab("Misc", settings_list_app);
    }
    //

    appView->setIcon(get_resource_path() + "download.png");
    brls::PopupFrame::open("Options", appView, "", "");
}