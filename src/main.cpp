#include <main.h>
#include <utils/blacklist.h>
#include <utils/favorites.h>
#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/notes.h>
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

std::string asset_path   = get_config_path("assets/");
std::string config__path = get_config_path("");

void export_resource(std::string folder, std::string src)
{
    folder += "/";
    create_directories(config__path + folder);

    if (!fs::exists(config__path + folder + src))
    {
        std::ifstream srcfile(("romfs:/" + src).c_str(), std::ios::binary);
        std::ofstream dstfile(config__path + folder + src, std::ios::binary);

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

    export_resource("forwarder", "HomebrewDetails_MultiForwarder.nsp");
}

int main(int argc, char* argv[])
{
    printf(argv[0]);
    printf("\n");

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    i18n::loadTranslations();

    std::string title = "Homebrew Details v" APP_VERSION;
    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    print_debug("Beginning setting prep.");
    read_settings();

    if (fs::exists(get_config_path("debug")))
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

    if (fs::exists(get_config_path("lock")))
    {
        print_debug("Issue Page");
        show_framed(new IssuePage());
    }
    else
    {
        print_debug("Intro Page");
        show_framed(new IntroPage());
    }

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.");

    return EXIT_SUCCESS;
}