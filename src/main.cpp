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

#include "intro_page.hpp"
#include "main_page.hpp"

#ifndef APP_VERSION
#error APP_VERSION define missing
#endif

int main(int argc, char* argv[])
{
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    std::string title = "Homebrew Details v" APP_VERSION;

    if (!brls::Application::init(title.c_str()))
    {
        brls::Logger::error("Unable to init Borealis application, Homebrew Details");
        return EXIT_FAILURE;
    }

    brls::Application::pushView(new IntroPage("Begin Scan"));

    // Run the app
    while (brls::Application::mainLoop())
        ;

    // Protect from crash
    //rootFrame->setParent(NULL);
    return EXIT_SUCCESS;
}