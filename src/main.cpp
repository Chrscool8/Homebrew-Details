#include <main.h>
#include <utils/launching.h>
#include <utils/nacp_utils.h>
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

void export_resource(std::string src)
{
    if (!fs::exists("sdmc:/config/homebrew_details/assets/" + src))
    {
        std::ifstream srcfile(("romfs:/" + src).c_str(), std::ios::binary);
        std::ofstream dstfile("sdmc:/config/homebrew_details/assets/" + src, std::ios::binary);

        dstfile << srcfile.rdbuf();
    }
}

void copy_resources()
{
    if (!fs::exists("sdmc:/config/"))
        fs::create_directory("sdmc:/config/");
    if (!fs::exists("sdmc:/config/homebrew_details/"))
        fs::create_directory("sdmc:/config/homebrew_details/");
    if (!fs::exists("sdmc:/config/homebrew_details/assets/"))
        fs::create_directory("sdmc:/config/homebrew_details/assets/");

    export_resource("arrows.jpg");
    export_resource("arrows_small.jpg");
    export_resource("download.jpg");
    export_resource("icon.jpg");
    export_resource("warning.jpg");
    export_resource("warning_arrows.jpg");
}

int main(int argc, char* argv[])
{
    print_debug(argv[0]);

    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    std::string title = "Homebrew Details v" APP_VERSION;
    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    copy_resources();

    file_load_settings();
    init_settings();
    set_setting(setting_nro_path, argv[0]);

    if (fs::exists("sdmc:/config/homebrew_details/debug"))
        set_setting(setting_debug, "true");

    init_online_info();
    check_for_updates();

    psmInitialize();

    if (fs::exists("sdmc:/config/homebrew_details/lock"))
    {
        print_debug("Issue Page\n");
        brls::Application::pushView(new IssuePage());
    }
    else
    {
        print_debug("Intro Page\n");
        brls::Application::pushView(new IntroPage("Begin Scan"));
    }

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.\n");

    return EXIT_SUCCESS;
}