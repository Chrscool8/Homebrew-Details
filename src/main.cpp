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

int main(int argc, char* argv[])
{
    printf(argv[0]);
    printf("\n");

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    i18n::loadTranslations();

    std::string title = std::string("") + "Homebrew Details Next  -  v" + APP_VERSION;
    printf((title + "\n").c_str());
    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    psmExit();
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

    std::string target = settings_get_value("meta", "exit to");
    envSetNextLoad(target.c_str(), (std::string("\"") + target + "\"").c_str());

    read_favorites();
    read_blacklist();
    read_notes();

    print_debug("Intro Page");
    brls::AppletFrame* intro = show_framed(new IntroPage());

    if (settings_get_value_true("meta", "debug"))
        title += " [Debug Mode]";
    intro->setTitle(title.c_str());

    intro->setIcon(get_resource_path() + "arrows.png");

    intro->registerAction("Welcome Screen", brls::Key::X, []() { show_first_time_panel(); return true; });
    intro->updateActionHint(brls::Key::X, "Welcome Screen");

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
    else
        check_for_updates();

    if (get_online_version_available())
    {
        brls::Application::notify("Update Available!\nPress R for more info.");

        intro->registerAction("Update Info", brls::Key::R, []() {
            show_update_panel();
            return true;
        });
        intro->updateActionHint(brls::Key::R, "Update Info");
    }

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.");

    return EXIT_SUCCESS;
}