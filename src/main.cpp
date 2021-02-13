#include <main.h>
#include <utils/blacklist.h>
#include <utils/favorites.h>
#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/notes.h>
#include <utils/panels.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>

//

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include <algorithm>
#include <borealis.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace i18n = brls::i18n; // for loadTranslations() and getStr()
using namespace i18n::literals; // for _i18n

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

namespace fs = std::filesystem;

nlohmann::json apps_info_json;
nlohmann::json store_info_json;

////
std::vector<std::string> favorites;
std::vector<std::string> blacklist;
std::map<std::string, brls::Image*> cached_thumbs;

void export_resource(std::string folder, std::string src)
{
    folder += "/";
    create_directories(get_config_path() + folder);

    if (!fs::exists(get_config_path() + folder + src))
    {
        std::ifstream srcfile(("romfs:/" + src).c_str(), std::ios::binary);
        std::ofstream dstfile(get_config_path() + folder + src, std::ios::binary);

        dstfile << srcfile.rdbuf();
    }
}

void copy_resources()
{
    export_resource("assets", "arrows.png");
    export_resource("assets", "arrows_small.png");
    export_resource("assets", "download.png");
    export_resource("assets", "icon.png");
    export_resource("assets", "warning.png");
    export_resource("assets", "warning_arrows.png");
    export_resource("assets", "style_list.png");
    export_resource("assets", "style_flow.png");
    export_resource("assets", "style_beta.png");
    export_resource("assets", "unknown.png");
    export_resource("assets", "credits.png");

    export_resource("forwarder", "HomebrewDetailsForwarder_v2.nsp");
}

int main(int argc, char* argv[])
{
    printf(argv[0]);
    printf("\n");

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    i18n::loadTranslations();

    std::string title = "Homebrew Details v" APP_VERSION;
    printf((title + "\n").c_str());
    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    psmInitialize();

    print_debug("Beginning setting prep.");
    read_settings();

    if (fs::exists(get_config_path() + "debug"))
    {
        settings_set_value("meta", "debug", "true");
        print_debug("Found debug file, setting as such.");
    }

    init_settings();
    settings_set_value("meta", "nro path", argv[0]);

    copy_resources();

    init_online_info();
    check_for_updates();

    std::string target = settings_get_value("meta", "exit to");
    envSetNextLoad(target.c_str(), (std::string("\"") + target + "\"").c_str());

    read_favorites();
    read_blacklist();
    read_notes();

    //if (fs::exists(get_config_path() + "lock"))
    //{
    //    print_debug("Issue Page");
    //    show_framed(new IssuePage());
    //}
    //else
    {
        print_debug("Intro Page");
        brls::AppletFrame* intro = show_framed(new IntroPage());

        std::string title = std::string("") + "Homebrew Details Next  -  v" + APP_VERSION;
        if (settings_get_value_true("meta", "debug"))
            title += " [Debug Mode]";
        intro->setTitle(title.c_str());

        intro->setIcon(get_resource_path() + "arrows.png");

        intro->registerAction("Welcome Screen", brls::Key::X, []() { show_first_time_panel(); return true; });
        intro->updateActionHint(brls::Key::X, "Welcome Screen");

        if (get_online_version_available())
        {
            brls::Application::notify("Update Available!\nPress R for more info.");

            intro->registerAction("Update Info", brls::Key::R, []() {
                show_update_panel();
                return true;
            });
            intro->updateActionHint(brls::Key::R, "Update Info");
        }

        if (!fs::exists(get_config_path() + "introduced"))
        {
            std::ofstream outputFile(get_config_path() + "introduced");
            if (outputFile)
            {
                outputFile << "introduced";
                outputFile.close();
            }

            show_first_time_panel();
        }
    }

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.");

    return EXIT_SUCCESS;
}