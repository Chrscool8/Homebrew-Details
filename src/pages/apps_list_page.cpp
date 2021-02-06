#include <dirent.h>
#include <main.h>
#include <utils/blacklist.h>
#include <utils/favorites.h>
#include <utils/launching.h>
#include <utils/modules.h>
#include <utils/notes.h>
#include <utils/panels.h>
#include <utils/scanning.h>
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

    print_debug("Building " + full_path);

    if (!fs::exists(full_path))
        full_path = "FILE MISSING";

    std::string fav = "";
    if (is_favorite(full_path))
        fav = symbol_star() + " ";

    std::string subvalue = settings_get_value("sort", "main");

    std::string entry_title = fav + json_load_value_string(app_json, "name");
    if (subvalue != "version")
        entry_title += "  -  v" + json_load_value_string(app_json, "version");

    brls::ListItem* this_entry = new brls::ListItem(entry_title, "", full_path);

    if (subvalue == "name" || subvalue == "full_path")
        subvalue = "author";

    this_entry->setValue(json_load_value_string(app_json, subvalue));

    this_entry->setThumbnail(load_image_cache(json_load_value_string(app_json, "full_path")));

    brls::Key key = brls::Key::A;
    if (settings_get_value("preferences", "control scheme") == "0")
        key = brls::Key::X;
    else if (settings_get_value("preferences", "control scheme") == "1")
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
    if (settings_get_value("preferences", "control scheme") == "0")
        key = brls::Key::A;
    else if (settings_get_value("preferences", "control scheme") == "1")
        key = brls::Key::X;

    this_entry->updateActionHint(key, "Details");
    this_entry->registerAction("Details", key, [this, full_path, app_json]() {
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

        {
            manageList->addView(new brls::Header("Other Actions"));
            brls::ListItem* fav_item = new brls::ListItem("");
            if (is_favorite(full_path))
            {
                fav_item->setLabel("Unfavorite App");
                fav_item->getClickEvent()->subscribe([this, full_path](brls::View* view) {
                    remove_favorite(full_path);
                    needs_refresh = true;
                    return true;
                });
            }
            else
            {
                fav_item->setLabel("Favorite App");
                fav_item->getClickEvent()->subscribe([this, full_path](brls::View* view) {
                    add_favorite(full_path);
                    needs_refresh = true;
                    return true;
                });
            }
            manageList->addView(fav_item);
        }

        appView->addTab("Manage", manageList);

        brls::List* appInfoList = new brls::List();
        appInfoList->addView(new brls::Header(".NRO File Info", false));
        add_list_entry("Name", json_load_value_string(app_json, "name"), "", appInfoList, 21);
        add_list_entry("Filename", json_load_value_string(app_json, "file_name"), std::string("Full Path:\n\n") + full_path, appInfoList, 21);
        add_list_entry("Author", json_load_value_string(app_json, "author"), "", appInfoList, 21);
        add_list_entry("Version", json_load_value_string(app_json, "version"), "", appInfoList, 21);
        add_list_entry("Size", to_megabytes(json_load_value_int(app_json, "size")) + " MB", "Exact Size:\n\n" + std::to_string(json_load_value_int(app_json, "size")) + " bytes", appInfoList, 21);
        add_list_entry("Favorite", json_load_value_string(app_json, "favorite"), "", appInfoList, 21);
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
                desc->invalidate();

                if (dest_string.empty())
                    note_header->collapse(true);
                else
                    note_header->expand(true);
                note_header->invalidate();
            });

            notesList->addView(notes_item);
            notesList->addView(note_header);
            notesList->addView(desc);
            appView->addTab("Notes", notesList);
        }

        brls::PopupFrame::open(json_load_value_string(app_json, "name"), appView, std::string("Author: ") + json_load_value_string(app_json, "author"), std::string("Version: ") + json_load_value_string(app_json, "version"));

        return true;
    });

    //

    this_entry->getFocusEvent()->subscribe([this, this_entry](brls::View* view) {
        print_debug("Focused");
        if (needs_refresh)
        {
            print_debug("Refresh!");
            needs_refresh = false;
            do_refresh    = true;
            ticker        = 0;
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
        //print_debug("Comparing " + comp_sort_main_a + " and " + comp_sort_main_b + " by " + sort_main + " and the result is " + std::to_string(comp_sort_main_a.compare(comp_sort_main_b) < 0));

        std::string comp_fav_a = bool_string(is_favorite(json_load_value_string(a, "full_path")));
        transform(comp_fav_a.begin(), comp_fav_a.end(), comp_fav_a.begin(), ::tolower);
        if (comp_fav_a == "---")
            comp_fav_a = "zzzzzzzzzzzzzz";
        std::string comp_fav_b = bool_string(is_favorite(json_load_value_string(b, "full_path")));
        transform(comp_fav_b.begin(), comp_fav_b.end(), comp_fav_b.begin(), ::tolower);
        if (comp_fav_b == "---")
            comp_fav_b = "zzzzzzzzzzzzzz";

        if (comp_fav_a != comp_fav_b)
        {
            return (!(comp_fav_a.compare(comp_fav_b) < 0));
        }
        else
        {
            if (settings_get_value("sort", "direction") == "descending")
            {
                nlohmann::json c = a;
                a                = b;
                b                = c;
            }

            std::string comp_sort_group_a = json_load_value_string(a, settings_get_value("sort", "grouping"));
            transform(comp_sort_group_a.begin(), comp_sort_group_a.end(), comp_sort_group_a.begin(), ::tolower);
            if (comp_sort_group_a == "---")
                comp_sort_group_a = "zzzzzzzzzzzzzz";
            std::string comp_sort_group_b = json_load_value_string(b, settings_get_value("sort", "grouping"));
            transform(comp_sort_group_b.begin(), comp_sort_group_b.end(), comp_sort_group_b.begin(), ::tolower);
            if (comp_sort_group_b == "---")
                comp_sort_group_b = "zzzzzzzzzzzzzz";

            if (comp_sort_group_a != comp_sort_group_b)
            {
                return (comp_sort_group_a.compare(comp_sort_group_b) < 0);
            }
            else
            {
                std::string comp_sort_main_a = json_load_value_string(a, sort_main);
                transform(comp_sort_main_a.begin(), comp_sort_main_a.end(), comp_sort_main_a.begin(), ::tolower);
                if (sort_main == "size")
                    comp_sort_main_a = string_pad_zeroes(comp_sort_main_a);
                if (comp_sort_main_a == "---")
                    comp_sort_main_a = "zzzzzzzzzzzzzz";
                std::string comp_sort_main_b = json_load_value_string(b, sort_main);
                transform(comp_sort_main_b.begin(), comp_sort_main_b.end(), comp_sort_main_b.begin(), ::tolower);
                if (sort_main == "size")
                    comp_sort_main_b = string_pad_zeroes(comp_sort_main_b);
                if (comp_sort_main_b == "---")
                    comp_sort_main_b = "zzzzzzzzzzzzzz";

                if (comp_sort_main_a != comp_sort_main_b)
                {
                    return (comp_sort_main_a.compare(comp_sort_main_b) < 0);
                }
                else
                {
                    std::string comp_sort_sub_a = json_load_value_string(a, sort_sub);
                    transform(comp_sort_sub_a.begin(), comp_sort_sub_a.end(), comp_sort_sub_a.begin(), ::tolower);
                    if (comp_sort_sub_a == "---")
                        comp_sort_sub_a = "zzzzzzzzzzzzzz";
                    std::string comp_sort_sub_b = json_load_value_string(b, sort_sub);
                    transform(comp_sort_sub_b.begin(), comp_sort_sub_b.end(), comp_sort_sub_b.begin(), ::tolower);
                    if (comp_sort_sub_b == "---")
                        comp_sort_sub_b = "zzzzzzzzzzzzzz";

                    if (comp_sort_sub_a != comp_sort_sub_b)
                    {
                        return (comp_sort_sub_a).compare(comp_sort_sub_b) < 0;
                    }
                    else
                    {
                        return (json_load_value_string(a, "version")).compare(json_load_value_string(b, "version")) < 0;
                    }
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
    dialogItem->setChecked(settings_get_value("sort", "main") == sort_name);
    dialogItem->getClickEvent()->subscribe([this, dialogItem, sort_name, secondary_sort](brls::View* view) {
        settings_set_value("sort", "main", sort_name);
        settings_set_value("sort", "secondary", secondary_sort);

        needs_refresh = true;

        brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
        for (size_t i = 0; i < list->getViewsCount(); i++)
            ((brls::ListItem*)(list->getChild(i)))->setChecked(false);
        dialogItem->setChecked(true);

        return true;
    });
    return dialogItem;
}

brls::ListItem* AppsListPage::create_sort_group_choice(std::string label, std::string sort_group)
{
    brls::ListItem* dialogItem = new brls::ListItem(label);
    dialogItem->setChecked(settings_get_value("sort", "grouping") == sort_group);
    dialogItem->getClickEvent()->subscribe([this, dialogItem, sort_group](brls::View* view) {
        settings_set_value("sort", "grouping", sort_group);

        needs_refresh = true;

        brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
        for (size_t i = 0; i < list->getViewsCount(); i++)
            ((brls::ListItem*)(list->getChild(i)))->setChecked(false);
        dialogItem->setChecked(true);

        return true;
    });
    return dialogItem;
}

brls::List* AppsListPage::build_app_list()
{
    print_debug("Building App List");

    brls::List* this_list = new brls::List();

    std::vector<nlohmann::json> apps_list = app_json_to_list(apps_info_json, settings_get_value("sort", "main"), settings_get_value("sort", "secondary"));

    this_list->addView(new brls::Header(std::to_string(apps_list.size()) + " Apps, sorted by " + settings_get_value("sort", "main") + " then " + settings_get_value("sort", "secondary") + ", " + settings_get_value("sort", "direction")));

    for (unsigned int i = 0; i < apps_list.size(); i++)
    {
        nlohmann::json entry = apps_list.at(i);

        if (!favorites.empty())
        {
            if (apps_list.size() > 1)
            {
                std::string header_name = " Favorites";

                if (i == 0)
                {
                    this_list->addView(new brls::Header(header_name));
                }
                else
                {
                    nlohmann::json entry_prev = apps_list.at(i - 1);

                    std::string str1 = bool_string(is_favorite(json_load_value_string(entry, "full_path")));
                    std::string str2 = bool_string(is_favorite(json_load_value_string(entry_prev, "full_path")));

                    if (str1 != str2)
                    {
                        this_list->addView(new brls::Header(" Non-Favorites"));
                    }
                }
            }
        }

        if (settings_get_value("sort", "grouping") != "")
        {
            if (apps_list.size() > 1)
            {
                std::string header_name = "  " + upper_first_letter(settings_get_value("sort", "grouping")) + ": " + upper_first_letter(json_load_value_string(entry, settings_get_value("sort", "grouping")));

                if (i == 0)
                {
                    this_list->addView(new brls::Header(header_name));
                }
                else
                {
                    nlohmann::json entry_prev = apps_list.at(i - 1);

                    std::string str1 = json_load_value_string(entry, settings_get_value("sort", "grouping"));
                    std::string str2 = json_load_value_string(entry_prev, settings_get_value("sort", "grouping"));

                    std::string str3 = bool_string(is_favorite(json_load_value_string(entry, "full_path")));
                    std::string str4 = bool_string(is_favorite(json_load_value_string(entry_prev, "full_path")));

                    if (str1 != str2 || str3 != str4)
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

    brls::Application::giveFocus(nullptr);

    brls::List* templist = new brls::List();
    templist->addView(new brls::ListItem(""));
    this->setContentView(templist);

    main_list = build_app_list();
    this->setContentView(main_list);

    unsigned int select_num = 0;
    for (unsigned int i = 0; i < main_list->getViewsCount(); i++)
    {
        brls::View* ent = main_list->getChild(i);
        if (ent->describe() == "N4brls8ListItemE")
        {
            select_num = i;
            break;
        }
    }

    if (select_num != 0)
        brls::Application::giveFocus(main_list->getChild(select_num));
}

AppsListPage::AppsListPage()
    : AppletFrame(false, false)
{
    std::string title = std::string("") + "Homebrew Details Next  -  v" + APP_VERSION;
    if (settings_get_value_true("meta", "debug"))
        title += " [Debug Mode]";
    this->setTitle(title.c_str());

    this->setIcon(get_resource_path() + "icon.png");

    main_list = build_app_list();
    this->setContentView(main_list);

    this->setActionAvailable(brls::Key::B, false);
    this->registerAction("Back", brls::Key::B, [this]() {
        return true;
    });

    this->registerAction("Settings", brls::Key::Y, [this]() {
        show_settings_panel();
        return true;
    });

    //this->registerAction("Favorite", brls::Key::LSTICK, [this]() {
    //    return true;
    //});

    this->updateActionHint(brls::Key::B, "");

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

        brls::List* list2          = new brls::List();
        brls::ListItem* dialogItem = new brls::ListItem("Ascending");
        dialogItem->setChecked(settings_get_value("sort", "direction") != "descending");
        dialogItem->getClickEvent()->subscribe([this, dialogItem](brls::View* view) {
            settings_set_value("sort", "direction", "ascending");

            brls::Sidebar* list4 = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list4->getChild(1)))->setChecked(false);

            dialogItem->setChecked(true);
            needs_refresh = true;
            return true;
        });
        list2->addView(dialogItem);
        //

        dialogItem = new brls::ListItem("Descending");
        dialogItem->setChecked(settings_get_value("sort", "direction") == "descending");
        dialogItem->getClickEvent()->subscribe([this, dialogItem](brls::View* view) {
            settings_set_value("sort", "direction", "descending");

            brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list->getChild(0)))->setChecked(false);

            dialogItem->setChecked(true);
            needs_refresh = true;

            return true;
        });
        list2->addView(dialogItem);

        appView->addTab("Sort Direction", list2);

        //

        brls::List* list3 = new brls::List();
        list3->addView(create_sort_group_choice("None", ""));
        list3->addView(create_sort_group_choice("By Author", "author"));
        list3->addView(create_sort_group_choice("By Category", "category"));
        list3->addView(create_sort_group_choice("From Appstore", "is_appstore"));

        appView->addTab("Grouping", list3);

        //

        appView->setIcon(get_resource_path() + "download.png");
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

    if (do_refresh)
    {
        if (ticker < 10)
        {
            ticker += 1;
            print_debug(std::to_string(ticker));
        }
        else
        {
            do_refresh = false;
            print_debug("Doing Refresh");
            refresh_list();
        }
    }

    AppletFrame::draw(vg, x, y, width, height, style, ctx);
    draw_status(this, x, y, width, height, style, ctx);
}