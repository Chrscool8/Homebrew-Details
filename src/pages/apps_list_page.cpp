#include <dirent.h>
#include <main.h>
#include <utils/launching.h>
#include <utils/modules.h>
#include <utils/notes.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <borealis.hpp>
#include <pages/apps_list_page.hpp>
#include <pages/info_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length = 21)
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

brls::ListItem* new_make_app_entry(app_entry* entry, bool is_appstore)
{
    brls::ListItem* this_entry = new brls::ListItem(entry->name, "", entry->full_path);
    this_entry->setValue("v" + entry->version);
    this_entry->setThumbnail(entry->icon, entry->icon_size);

    brls::Key key = brls::Key::A;
    if (get_setting(setting_control_scheme) == "0")
        key = brls::Key::X;
    else if (get_setting(setting_control_scheme) == "1")
        key = brls::Key::A;

    this_entry->updateActionHint(key, "Launch");
    this_entry->registerAction("Launch", key, [entry]() {
        print_debug("launch app");
        unsigned int r = launch_nro(entry->full_path, "\"" + entry->full_path + "\"");
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
    this_entry->registerAction("Details", key, [entry, this_entry, is_appstore]() {
        brls::TabFrame* appView = new brls::TabFrame();

        brls::List* manageList = new brls::List();
        manageList->addView(new brls::Header("File Management Actions", false));

        {
            brls::ListItem* launch_item = new brls::ListItem("Launch App");
            launch_item->getClickEvent()->subscribe([entry](brls::View* view) {
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
            delete_item->getClickEvent()->subscribe([entry, appView](brls::View* view) {
                brls::Dialog* dialog                     = new brls::Dialog("Are you sure you want to delete the following file? This action cannot be undone.\n\n" + entry->full_path);
                brls::GenericEvent::Callback yesCallback = [dialog, entry, appView](brls::View* view) {
                    if (remove(entry->full_path.c_str()) != 0)
                        brls::Application::notify("Issue removing file");
                    else
                    {
                        std::string _folder = folder_of_file(entry->full_path);
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

    return this_entry;
}

AppsListPage::AppsListPage()
    : AppletFrame(false, false)
{
    brls::List* this_list = new brls::List();

    if (local_apps.empty())
        this_list->addView(new brls::ListItem("No Apps Found."));

    for (unsigned int i = 0; i < local_apps.size(); i++)
    {
        app_entry* current = &local_apps.at(i);
        this_list->addView(new_make_app_entry(current, false));
    }

    this->setIcon(get_resource_path("icon.png"));

    std::string title = "Homebrew Details v" + get_setting(setting_local_version);
    if (get_setting_true(setting_debug))
        title += " [Debug Mode]";
    this->setTitle(title.c_str());

    this->setContentView(this_list);

    if (get_online_version_available())
    {
        this->registerAction("Update Info", brls::Key::L, [&]() {
            brls::TabFrame* appView = new brls::TabFrame();
            appView->sidebar->setWidth(1000);
            std::string vers = " v" + get_setting(setting_local_version) + "  " + " " + symbol_rightarrow() + " " + "  v" + get_online_version_number() + "\n\n";

            appView->sidebar->addView(new brls::Header("Update Actions", false));
            brls::ListItem* dialogItem = new brls::ListItem("Update Wizard");
            dialogItem->getClickEvent()->subscribe([&](brls::View* view) {
                brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                stagedFrame->setTitle("Update Wizard");
                stagedFrame->setIcon(get_resource_path("icon.png"));
                stagedFrame->setActionAvailable(brls::Key::B, false);

                stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_intro));
                stagedFrame->addStage(new UpdatingPage(stagedFrame));
                stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_done));

                brls::Application::pushView(stagedFrame);
            });

            appView->sidebar->addView(dialogItem);
            appView->sidebar->addView(new brls::Label(brls::LabelStyle::REGULAR, " \n ", true));
            appView->sidebar->addView(new brls::Header("New Version Details", false));
            appView->sidebar->addView(add_list_entry("Online Version", "v" + get_online_version_number(), "", NULL, 40));
            appView->sidebar->addView(add_list_entry("Title", get_online_version_name(), "", NULL, 40));
            appView->sidebar->addView(add_list_entry("Description", get_online_version_description(), "", NULL, 40));
            appView->sidebar->addView(add_list_entry("Date", get_online_version_date(), "", NULL, 40));

            appView->setIcon(get_resource_path("download.png"));
            brls::PopupFrame::open("Update Info", appView, vers, "");
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