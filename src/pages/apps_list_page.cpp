#include <dirent.h>
#include <main.h>
#include <utils/launching.h>
#include <utils/modules.h>
#include <utils/notes.h>
#include <utils/panels.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <borealis.hpp>
#include <pages/apps_list_page.hpp>
#include <pages/info_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

brls::ListItem* new_new_make_app_entry(nlohmann::json app_json)
{
    std::string full_path = json_load_value_string(app_json, "full_path");

    if (!fs::exists(full_path))
        full_path = "FILE MISSING";

    brls::ListItem* this_entry = new brls::ListItem(json_load_value_string(app_json, "name") + "  -  " + "v" + json_load_value_string(app_json, "version"), "", full_path);

    this_entry->setValue(json_load_value_string(app_json, get_setting(setting_sort_type)));

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

        std::string _a = json_load_value_string(a, sort_main);
        transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
        std::string _b = json_load_value_string(b, sort_main);
        transform(_b.begin(), _b.end(), _b.begin(), ::tolower);

        //print_debug("Comparing " + _a + " and " + _b + " by " + sort_main + " and the result is " + std::to_string(_a.compare(_b) < 0));

        if (_a != _b)
        {
            return (_a.compare(_b) < 0);
        }
        else
        {
            std::string _c = json_load_value_string(a, sort_sub);
            transform(_c.begin(), _c.end(), _c.begin(), ::tolower);
            std::string _d = json_load_value_string(b, sort_sub);
            transform(_d.begin(), _d.end(), _d.begin(), ::tolower);

            if (_c != _d)
            {
                return (json_load_value_string(a, sort_sub)).compare(json_load_value_string(b, sort_sub)) < 0;
            }
            else
            {
                return (json_load_value_string(a, "version")).compare(json_load_value_string(b, "version")) < 0;
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

brls::ListItem* create_sort_type_choice(std::string label, std::string sort_name, std::string secondary_sort)
{
    brls::ListItem* dialogItem = new brls::ListItem(label);
    dialogItem->setChecked(get_setting(setting_sort_type) == sort_name);
    dialogItem->getClickEvent()->subscribe([dialogItem, sort_name, secondary_sort](brls::View* view) {
        set_setting(setting_sort_type, sort_name);
        set_setting(setting_sort_type_2, secondary_sort);

        brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
        for (unsigned int i = 0; i < list->getViewsCount(); i++)
            ((brls::ListItem*)(list->getChild(i)))->setChecked(false);
        dialogItem->setChecked(true);

        return true;
    });
    return dialogItem;
}

AppsListPage::AppsListPage()
    : AppletFrame(false, false)
{
    std::string title = "Homebrew Details v" + get_setting(setting_local_version);
    if (get_setting_true(setting_debug))
        title += " [Debug Mode]";
    this->setTitle(title.c_str());

    this->setIcon(get_resource_path("icon.png"));

    brls::List* this_list = new brls::List();

    std::vector<nlohmann::json> apps_list = app_json_to_list(apps_info_json, get_setting(setting_sort_type), get_setting(setting_sort_type_2));

    this_list->addView(new brls::Header(std::to_string(apps_list.size()) + " Apps, sorted by " + get_setting(setting_sort_type) + " then " + get_setting(setting_sort_type_2) + ", " + get_setting(setting_sort_direction)));

    for (unsigned int i = 0; i < apps_list.size(); i++)
        this_list->addView(new_new_make_app_entry(apps_list.at(i)));

    //
    if (apps_list.empty())
        this_list->addView(new brls::ListItem("No Apps Found."));

    this->setContentView(this_list);

    //sort panel

    this->registerAction("Sorting Options", brls::Key::L, []() {
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
        brls::ListItem* dialogItem = new brls::ListItem("ascending");
        dialogItem->setChecked(get_setting(setting_sort_direction) != "ascending");
        dialogItem->getClickEvent()->subscribe([dialogItem](brls::View* view) {
            set_setting(setting_sort_direction, "ascending");

            brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list->getChild(1)))->setChecked(false);

            dialogItem->setChecked(true);

            return true;
        });
        list->addView(dialogItem);
        //

        dialogItem = new brls::ListItem("descending");
        dialogItem->setChecked(get_setting(setting_sort_direction) == "descending");
        dialogItem->getClickEvent()->subscribe([dialogItem](brls::View* view) {
            set_setting(setting_sort_direction, "descending");

            brls::Sidebar* list = (brls::Sidebar*)dialogItem->getParent();
            ((brls::ListItem*)(list->getChild(0)))->setChecked(false);

            dialogItem->setChecked(true);

            return true;
        });
        list->addView(dialogItem);

        appView->addTab("Sort Direction", list);

        //

        appView->setIcon(get_resource_path("download.png"));
        brls::PopupFrame::open("Sorting Options", appView, "", "");

        return true;
    });

    if (get_online_version_available())
    {
        brls::Application::notify("Update Available!\nPress L for more info.");

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