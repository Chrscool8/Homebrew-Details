#include <main.h>
#include <utils/blacklist.h>
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

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

namespace fs = std::filesystem;

////
std::vector<std::string> favorites;
std::vector<std::string> blacklist;
std::vector<app_entry> local_apps;
std::vector<app_entry> store_apps;
std::vector<app_entry> store_file_data;

std::string asset_path   = "sdmc:/config/homebrew_details/assets/";
std::string config__path = "sdmc:/config/homebrew_details/";

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
    export_resource("assets", "arrows.jpg");
    export_resource("assets", "arrows_small.jpg");
    export_resource("assets", "download.jpg");
    export_resource("assets", "icon.jpg");
    export_resource("assets", "warning.jpg");
    export_resource("assets", "warning_arrows.jpg");

    export_resource("forwarders", "HomebrewDetailsForwarder.nsp");
    export_resource("forwarders", "HomebrewDetailsForwarderAlt.nsp");
}

int main(int argc, char* argv[])
{
    printf(argv[0]);

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    std::string title = "Homebrew Details v" APP_VERSION;
    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    file_load_settings();
    init_settings();
    set_setting(setting_nro_path, argv[0]);

    copy_resources();

    if (fs::exists("sdmc:/config/homebrew_details/debug"))
        set_setting(setting_debug, "true");

    init_online_info();
    check_for_updates();

    psmInitialize();

    if (fs::exists("sdmc:/config/homebrew_details/lock"))
    {
        print_debug("Issue Page");
        brls::AppletFrame* frame = new brls::AppletFrame(false, false);
        frame->setContentView(new IssuePage());
        brls::Application::pushView(frame);
    }
    else
    {
        print_debug("Intro Page");
        brls::AppletFrame* frame = new brls::AppletFrame(false, false);
        frame->setContentView(new IntroPage("Begin Scan"));
        brls::Application::pushView(frame);
    }

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.");

    psmExit();

    return EXIT_SUCCESS;
}