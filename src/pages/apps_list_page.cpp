#include <dirent.h>
#include <main.h>
#include <utils/blacklist.h>
#include <utils/launching.h>
#include <utils/modules.h>
#include <utils/notes.h>
#include <utils/panels.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <borealis.hpp>
#include <nlohmann/json.hpp>
#include <pages/apps_list_page.hpp>
#include <pages/info_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

brls::ListItem* AppsListPage::new_new_make_app_entry(nlohmann::json app_json)
{
    std::string full_path = json_load_value_string(app_json, "full_path");

    if (!fs::exists(full_path))
        full_path = "FILE MISSING";

    brls::ListItem* this_entry = new brls::ListItem(json_load_value_string(app_json, "name") + "  -  " + "v" + json_load_value_string(app_json, "version"), "", full_path);

    //this_entry->setValue(json_load_value_string(app_json, get_setting(setting_sort_type)));
    this_entry->setValue(json_load_value_string(app_json, "author"));

    std::string icon_path = get_resource_path("unknown.png");

    std::string encoded_icon_path = get_cache_path(base64_encode(json_load_value_string(app_json, "name") + json_load_value_string(app_json, "version")) + ".jpg");

    if (fs::exists(encoded_icon_path))
        icon_path = encoded_icon_path;

    this_entry->setThumbnail(icon_path);

    brls::Key key = brls::Key::A;
    if (get_setting(setting_control_scheme) == "0")
        key = brls::Key::X;
    else if (get_setting(setting_control_scheme) == "1")
        key = brls::Key::A;

    this_entry->updateActionHint(key, "Launch");
    this_entry->registerAction("Launch", key, [full_path]() {
        print_debug("launch app");
        unsigned int r = launch_nro(full_path, "\"" + full_path + "\"");
        print_debug("r: " + std::to_string(r));
        if (R_FAILED(r))
        {
            print_debug("Uh oh.");
        }
        else
        {
            brls::Application::quit();
        }

        return true;
    });

    key = brls::Key::X;
    if (get_setting(setting_control_scheme) == "0")
        key = brls::Key::A;
    else if (get_setting(setting_control_scheme) == "1")
        key = brls::Key::X;

    this_entry->updateActionHint(key, "Details");
    this_entry->registerAction("Details", key, [full_path, app_json, icon_path]() {
        brls::TabFrame* appView = new brls::TabFrame();

        brls::List* manageList = new brls::List();
        manageList->addView(new brls::Header("File Management Actions", false));

        {
            brls::ListItem* launch_item = new brls::ListItem("Launch App");
            launch_item->getClickEvent()->subscribe([full_path](brls::View* view) {
                print_debug("launch app");
                unsigned int r = launch_nro(full_path, "\"" + full_path + "\"");
                print_debug("r: " + std::to_string(r));
                if (R_FAILED(r))
                {
                    print_debug("Uh oh.");
                }
                else
                {
                    brls::Application::quit();
                }
            });
            manageList->addView(launch_item);
        }

        //
        if (json_load_value_string(app_json, "is_appstore") == "true")
        {
            brls::ListItem* move_item = new brls::ListItem("Move App");
            move_item->getClickEvent()->subscribe([full_path](brls::View* view) {
                std::string dest_string = get_keyboard_input(full_path);

                if (to_lower(dest_string) == to_lower(full_path))
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

                    brls::Dialog* confirm_dialog             = new brls::Dialog("Are you sure you want to move the following file? This action cannot be undone.\n\n" + full_path + "\n" + symbol_downarrow() + "\n" + dest_string);
                    brls::GenericEvent::Callback yesCallback = [full_path, dest_string, confirm_dialog](brls::View* view) {
                        if (rename(full_path.c_str(), dest_string.c_str()) != 0)
                            brls::Application::notify("Issue moving file");
                        else
                        {
                            brls::Application::notify("File successfully moved");
                            //purge_entry(entry);
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
            copy_item->getClickEvent()->subscribe([full_path](brls::View* view) {
                std::string dest_string = get_keyboard_input(full_path);

                if (to_lower(dest_string) == to_lower(full_path))
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

                    brls::Dialog* confirm_dialog             = new brls::Dialog("Are you sure you want to copy to the following file? This action cannot be undone.\n\n" + full_path + "\n" + symbol_downarrow() + "\n" + dest_string);
                    brls::GenericEvent::Callback yesCallback = [full_path, dest_string, confirm_dialog](brls::View* view) {
                        if (copy_file(full_path.c_str(), dest_string.c_str()))
                        {
                            brls::Application::notify("File successfully copied");
                            //purge_entry(entry);
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
            delete_item->getClickEvent()->subscribe([full_path](brls::View* view) {
                brls::Dialog* dialog                     = new brls::Dialog("Are you sure you want to delete the following file? This action cannot be undone.\n\n" + full_path);
                brls::GenericEvent::Callback yesCallback = [full_path, dialog](brls::View* view) {
                    if (remove(full_path.c_str()) != 0)
                        brls::Application::notify("Issue removing file");
                    else
                    {
                        std::string _folder = folder_of_file(full_path);
                        print_debug("_folder " + _folder);

                        const char* basePath = _folder.c_str();

                        struct dirent* dp;
                        DIR* dir = opendir(basePath);

                        int num_files = 0;
                        while ((dp = readdir(dir)) != NULL)
                        {
                            print_debug("content: " + std::string(basePath) + std::string(dp->d_name));
                            num_files += 1;
                        }

                        closedir(dir);

                        print_debug("num things: " + std::to_string(num_files));

                        if (num_files == 0)
                        {
                            int res = rmdir(_folder.c_str());
                            print_debug("res " + std::to_string(res));

                            print_debug("No other files, removing " + _folder);
                        }
                        else
                            print_debug("Other files, not removing " + _folder);

                        brls::Application::notify("File successfully deleted");
                        //purge_entry(entry);
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
        add_list_entry("Name", json_load_value_string(app_json, "name"), "", appInfoList, 21);
        add_list_entry("Filename", json_load_value_string(app_json, "file_name"), std::string("Full Path:\n\n") + full_path, appInfoList, 21);
        add_list_entry("Author", json_load_value_string(app_json, "author"), "", appInfoList, 21);
        add_list_entry("Version", json_load_value_string(app_json, "version"), "", appInfoList, 21);
        add_list_entry("Size", to_megabytes(json_load_value_int(app_json, "size")) + " MB", "Exact Size:\n\n" + std::to_string(json_load_value_int(app_json, "size")) + " bytes", appInfoList, 21);
        //add_list_entry("Icon Size", std::to_string(json_load_value_int(app_json, "icon_size")), "", appInfoList, 21);
        appView->addTab("File Info", appInfoList);

        brls::List* appStoreInfoList = new brls::List();
        appStoreInfoList->addView(new brls::Header("App Store Info", false));

        add_list_entry("From Appstore", json_load_value_string(app_json, "is_appstore"), "", appStoreInfoList, 21);

        add_list_entry("URL", json_load_value_string(app_json, "url"), "", appStoreInfoList, 21);
        add_list_entry("Category", json_load_value_string(app_json, "category"), "", appStoreInfoList, 21);
        add_list_entry("License", json_load_value_string(app_json, "license"), "", appStoreInfoList, 21);
        add_list_entry("Description", json_load_value_string(app_json, "description"), "", appStoreInfoList, 21);
        add_list_entry("Summary", json_load_value_string(app_json, "summary"), "", appStoreInfoList, 21);
        add_list_entry("Changelog", json_load_value_string(app_json, "changelog"), "", appStoreInfoList, 21);
        appView->addTab("App Store Info", appStoreInfoList);

        {
            brls::List* notesList      = new brls::List();
            brls::ListItem* notes_item = new brls::ListItem("Edit Notes");
            brls::Label* desc          = new brls::Label(brls::LabelStyle::DESCRIPTION, notes_get_value(json_load_value_string(app_json, "file_name")), true);
            brls::Label* note_header   = new brls::Label(brls::LabelStyle::REGULAR, "Notes:", false);
            note_header->setVerticalAlign(NVGalign::NVG_ALIGN_TOP);

            if (notes_get_value(json_load_value_string(app_json, "file_name")).empty())
                note_header->collapse(true);
            else
                note_header->expand(true);

            notes_item->getClickEvent()->subscribe([app_json, desc, note_header](brls::View* view) {
                std::string dest_string = get_keyboard_input(notes_get_value(json_load_value_string(app_json, "file_name")));
                notes_set_value(json_load_value_string(app_json, "file_name"), dest_string);
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

        brls::PopupFrame::open(json_load_value_string(app_json, "name"), icon_path, appView, std::string("Author: ") + json_load_value_string(app_json, "author"), std::string("Version: ") + json_load_value_string(app_json, "version"));

        return true;
    });

    //

    this_entry->getFocusEvent()->subscribe([this, this_entry](brls::View* view) {
        if (needs_refresh)
        {

            //this_entry->onFocusLost();
            //brls::Application::giveFocus(nullptr);
            //refresh_list();
        }
        return true;
    });

    return this_entry;
}

struct AppComparator
{
    explicit AppComparator(std::string sort_main_, std::string sort_sub_)
    {
        sort_main = sort_main_;
        sort_sub  = sort_sub_;
    }

    std::string sort_main;
    std::string sort_sub;

    bool operator()(nlohmann::json a, nlohmann::json b) const
    {
        if (get_setting(setting_sort_direction) == "descending")
        {
            nlohmann::json c = a;
            a                = b;
            b                = c;
        }

        //print_debug("Comparing " + _a + " and " + _b + " by " + sort_main + " and the result is " + std::to_string(_a.compare(_b) < 0));

        std::string _e = json_load_value_string(a, get_setting(setting_sort_group));
        transform(_e.begin(), _e.end(), _e.begin(), ::tolower);
        if (_e == "---")
            _e = "zzzzzzzzzzzzzz";
        std::string _f = json_load_value_string(b, get_setting(setting_sort_group));
        transform(_f.begin(), _f.end(), _f.begin(), ::tolower);
        if (_f == "---")
            _f = "zzzzzzzzzzzzzz";

        if (_e != _f)
        {
            return (_e.compare(_f) < 0);
        }
        else
        {

            std::string _a = json_load_value_string(a, sort_main);
            transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
            if (_a == "---")
                _a = "zzzzzzzzzzzzzz";
            std::string _b = json_load_value_string(b, sort_main);
            transform(_b.begin(), _b.end(), _b.begin(), ::tolower);
            if (_b == "---")
                _b = "zzzzzzzzzzzzzz";

            if (_a != _b)
            {
                return (_a.compare(_b) < 0);
            }
            else
            {
                std::string _c = json_load_value_string(a, sort_sub);
                transform(_c.begin(), _c.end(), _c.begin(), ::tolower);
                if (_c == "---")
                    _c = "zzzzzzzzzzzzzz";
                std::string _d = json_load_value_string(b, sort_sub);
                transform(_d.begin(), _d.end(), _d.begin(), ::tolower);
                if (_d == "---")
                    _d = "zzzzzzzzzzzzzz";

                if (_c != _d)
                {
                    return (_c).compare(_d) < 0;
                }
                else
                {
                    return (json_load_value_string(a, "version")).compare(json_load_value_string(b, "version")) < 0;
                }
            }
        }
    }
};

std::vector<nlohmann::json> app_json_to_list(nlohmann::json json, std::string sort_by, std::string sort_by_secondary)
{
    std::vector<nlohmann::json> list;

    for (auto it = json.begin(); it != json.end(); ++it)
        list.push_back(it.value());

    sort(list.begin(), list.end(), AppComparator(sort_by, sort_by_secondary));

    return list;
}

brls::ListItem* AppsListPage::create_sort_type_choice(std::string label, std::string sort_name, std::string secondary_sort)
{
    brls::ListItem* dialogItem = new brls::ListItem(label);
    dialogItem->setChecked(get_setting(setting_sort_type) == sort_name);
    dialogItem->getClickEvent()->subscribe([this, dialogItem, sort_name, secondary_sort](brls::View* view) {
        set_setting(setting_sort_type, sort_name);
        set_setting(setting_sort_type_2, secondary_sort);

        needs_refresh = true;
        brls::Application::popView();
        refresh_list();

        brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
        for (unsigned int i = 0; i < list->getViewsCount(); i++)
            ((brls::ListItem*)(list->getChild(i)))->setChecked(false);
        dialogItem->setChecked(true);

        return true;
    });
    return dialogItem;
}

brls::ListItem* AppsListPage::create_sort_group_choice(std::string label, std::string sort_group)
{
    brls::ListItem* dialogItem = new brls::ListItem(label);
    dialogItem->setChecked(get_setting(setting_sort_group) == sort_group);
    dialogItem->getClickEvent()->subscribe([this, dialogItem, sort_group](brls::View* view) {
        set_setting(setting_sort_group, sort_group);

        needs_refresh = true;
        brls::Application::popView();
        refresh_list();

        brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
        for (unsigned int i = 0; i < list->getViewsCount(); i++)
            ((brls::ListItem*)(list->getChild(i)))->setChecked(false);
        dialogItem->setChecked(true);

        return true;
    });
    return dialogItem;
}

brls::List* AppsListPage::build_app_list()
{
    brls::List* this_list = new brls::List();

    std::vector<nlohmann::json> apps_list = app_json_to_list(apps_info_json, get_setting(setting_sort_type), get_setting(setting_sort_type_2));

    this_list->addView(new brls::Header(std::to_string(apps_list.size()) + " Apps, sorted by " + get_setting(setting_sort_type) + " then " + get_setting(setting_sort_type_2) + ", " + get_setting(setting_sort_direction)));

    for (unsigned int i = 0; i < apps_list.size(); i++)
    {
        nlohmann::json entry = apps_list.at(i);

        if (get_setting(setting_sort_group) != "")
        {
            if (apps_list.size() > 1)
            {
                std::string header_name = upper_first_letter(get_setting(setting_sort_group)) + ": " + upper_first_letter(json_load_value_string(entry, get_setting(setting_sort_group)));

                if (i == 0)
                {
                    this_list->addView(new brls::Header(header_name));
                }
                else
                {
                    nlohmann::json entry_prev = apps_list.at(i - 1);

                    std::string str1 = json_load_value_string(entry, get_setting(setting_sort_group));
                    std::string str2 = json_load_value_string(entry_prev, get_setting(setting_sort_group));

                    if (str1 != str2)
                        this_list->addView(new brls::Header(header_name));
                }
            }
        }

        this_list->addView(new_new_make_app_entry(entry));
    }

    if (apps_list.empty())
        this_list->addView(new brls::ListItem("No Apps Found."));

    return this_list;
}

void AppsListPage::refresh_list()
{
    print_debug("Refreshing App List");

    needs_refresh = false;

    brls::Application::giveFocus(nullptr);
    this->setContentView(new brls::ListItem(""));

    main_list->clear();
    main_list = build_app_list();
    this->setContentView(main_list);
    if (get_setting(setting_sort_group) != "")
        brls::Application::giveFocus(main_list->getChild(2));
    else
        brls::Application::giveFocus(main_list->getChild(1));
}

AppsListPage::AppsListPage()
    : AppletFrame(false, false)
{
    std::string title = "Homebrew Details v" + get_setting(setting_local_version);
    if (get_setting_true(setting_debug))
        title += " [Debug Mode]";
    this->setTitle(title.c_str());

    this->setIcon(get_resource_path("icon.png"));

    main_list = build_app_list();
    this->setContentView(main_list);

    this->setActionAvailable(brls::Key::B, false);
    this->registerAction("Back", brls::Key::B, [this]() {
        return true;
    });

    this->registerAction("Settings", brls::Key::Y, [this]() {
        brls::TabFrame* appView = new brls::TabFrame();

        {
            print_debug("Settings.");

            brls::List* settings_list_scan = new brls::List();
            settings_list_scan->addView(new brls::Header("Scan Settings"));

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
            settings_list_scan->addView(autoscan_switch);

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
            settings_list_scan->addView(layerSelectItem);
            settings_list_scan->addView(item_scan_switch);
            settings_list_scan->addView(item_scan_switch_subs);
            settings_list_scan->addView(item_scan_root);

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
            //settings_list->addView(new brls::Header("Control Settings"));

            brls::SelectListItem* controlSelectItem = new brls::SelectListItem("Control Settings", { "A: Details; X: Launch", "A: Launch; X: Details" }, std::stoi(get_setting(setting_control_scheme)), "Takes full effect on next launch.");
            controlSelectItem->getValueSelectedEvent()->subscribe([](size_t selection) {
                set_setting(setting_control_scheme, std::to_string(selection));
            });
            settings_list_controls->addView(controlSelectItem);

            appView->addTab("Controls", settings_list_controls);
        }
        //

        {
            brls::List* settings_list_app = new brls::List();
            settings_list_app->addView(new brls::Header("App Settings"));

            brls::SelectListItem* exitToItem = new brls::SelectListItem("Exit To", { "sdmc:/hbmenu.nro", get_setting(setting_nro_path) });
            exitToItem->setValue(get_setting(setting_exit_to));
            exitToItem->getValueSelectedEvent()->subscribe([](size_t selection) {
                if (selection == 0)
                    set_setting(setting_exit_to, "sdmc:/hbmenu.nro");
                else if (selection == 1)
                    set_setting(setting_exit_to, get_setting(setting_nro_path));

                std::string target = get_setting(setting_exit_to);
                envSetNextLoad(target.c_str(), (std::string("\"") + target + "\"").c_str());
            });
            settings_list_app->addView(exitToItem);

            print_debug("Misc.");
            settings_list_app->addView(new brls::Header("Misc. Settings"));

            brls::ListItem* debug_switch = new brls::ListItem("Debug Mode", "Takes full effect on next launch.");
            debug_switch->setChecked(get_setting_true(setting_debug));
            debug_switch->updateActionHint(brls::Key::A, "Toggle");
            debug_switch->getClickEvent()->subscribe([debug_switch](brls::View* view) {
                if (get_setting_true(setting_debug))
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
            settings_list_app->addView(debug_switch);
            appView->addTab("Misc", settings_list_app);
        }
        //

        appView->setIcon(get_resource_path("download.png"));
        brls::PopupFrame::open("Options", appView, "", "");

        return true;
    });

    //sort panel
    this->registerAction("Sorting Options", brls::Key::L, [this]() {
        brls::TabFrame* appView = new brls::TabFrame();

        brls::List* list = new brls::List();
        list->addView(create_sort_type_choice("By Name", "name", "version"));
        list->addView(create_sort_type_choice("By Full Path", "full_path", "name"));
        list->addView(create_sort_type_choice("By Author", "author", "name"));
        list->addView(create_sort_type_choice("By Size", "size", "name"));
        list->addView(create_sort_type_choice("By Category", "category", "name"));
        appView->addTab("Sort Type", list);

        //

        list                       = new brls::List();
        brls::ListItem* dialogItem = new brls::ListItem("Ascending");
        dialogItem->setChecked(get_setting(setting_sort_direction) != "descending");
        dialogItem->getClickEvent()->subscribe([this, dialogItem](brls::View* view) {
            set_setting(setting_sort_direction, "ascending");

            brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list->getChild(1)))->setChecked(false);

            dialogItem->setChecked(true);
            needs_refresh = true;
            brls::Application::popView();
            refresh_list();

            return true;
        });
        list->addView(dialogItem);
        //

        dialogItem = new brls::ListItem("Descending");
        dialogItem->setChecked(get_setting(setting_sort_direction) == "descending");
        dialogItem->getClickEvent()->subscribe([this, dialogItem](brls::View* view) {
            set_setting(setting_sort_direction, "descending");

            brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list->getChild(0)))->setChecked(false);

            dialogItem->setChecked(true);
            needs_refresh = true;
            brls::Application::popView();
            refresh_list();

            return true;
        });
        list->addView(dialogItem);

        appView->addTab("Sort Direction", list);

        //

        list = new brls::List();
        list->addView(create_sort_group_choice("None", ""));
        list->addView(create_sort_group_choice("By Author", "author"));
        list->addView(create_sort_group_choice("By Category", "category"));
        list->addView(create_sort_group_choice("From Appstore", "is_appstore"));

        appView->addTab("Grouping", list);

        //

        appView->setIcon(get_resource_path("download.png"));
        brls::PopupFrame::open("Sorting Options", appView, "", "");

        return true;
    });

    if (get_online_version_available())
    {
        brls::Application::notify("Update Available!\nPress R for more info.");

        this->registerAction("Update Info", brls::Key::R, []() {
            show_update_panel();
            return true;
        });
    }
}

AppsListPage::~AppsListPage()
{
}

void AppsListPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    AppletFrame::draw(vg, x, y, width, height, style, ctx);
    draw_status(this, x, y, width, height, style, ctx);
}