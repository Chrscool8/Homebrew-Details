#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>
#include <pages/update_page.hpp>

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

    file_load_settings();
    init_settings();
    set_setting(setting_nro_path, argv[0]);

    if (fs::exists("sdmc:/config/homebrew_details/lock"))
        brls::Application::pushView(new IssuePage());
    else
        brls::Application::pushView(new IntroPage("Begin Scan"));

    // Run the app
    while (brls::Application::mainLoop())
        ;

    print_debug("Main loop end.\n");

    // Protect from crash
    //rootFrame->setParent(NULL);
    return EXIT_SUCCESS;
}