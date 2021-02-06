#include <utils/blacklist.h>
#include <utils/favorites.h>
#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/notes.h>
#include <utils/reboot_to_payload.h>
#include <utils/scanning.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <pages/apps_list_page.hpp>
#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/layout_select_page.hpp>
#include <pages/main_page.hpp>
#include <pages/scanning_page.hpp>
//

#include <math.h>
#include <sys/select.h>

#include <chrono>
#include <thread>

//
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <borealis.hpp>
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "switch/services/psm.h"

void ScanningPage::thread_scan()
{
    print_debug("scan go");

    if (settings_get_value_true("internal", "invalidate cache"))
    {
        settings_set_value("internal", "invalidate cache", "false");
        remove(get_apps_cache_file().c_str());
        settings_set_value("scan", "settings changed", "false");
    }

    from_cache = std::filesystem::exists(get_apps_cache_file());

    if (from_cache)
    {
        std::ifstream i(get_apps_cache_file());
        apps_info_json.clear();
        i >> apps_info_json;
    }
    else
    {
        new_load_all_apps();
        new_read_store_apps();
    }

    print_debug("scan thread end");

    settings_set_value("history", "previous number of files", std::to_string(file_count));
    scanprog.complete       = true;
    scanprog.prev_num_files = file_count;
}

ScanningPage::ScanningPage()
{
    std::ofstream outputFile(get_config_path() + "lock");
    if (outputFile)
    {
        outputFile << "lock";
        outputFile.close();
    }

    //

    finished_download = false;

    go         = false;
    asked      = false;
    short_wait = 0;
    continued  = false;

    file_count = 0;

    scanprog.progress   = 0;
    scanprog.complete   = false;
    scanprog.success    = false;
    scanprog.scanning   = false;
    scanprog.end_thread = false;

    print_debug(settings_get_value("history", "previous number of files") + " previous files");

    if ((settings_get_value("history", "previous number of files")) != "---")
        scanprog.prev_num_files = std::stoi(settings_get_value("history", "previous number of files"));
    else
        scanprog.prev_num_files = -1;

    // Label
    this->progressDisp = new brls::ProgressDisplay();
    this->progressDisp->setProgress(0, 1);
    this->progressDisp->setParent(this);
    this->label = new brls::Label(brls::LabelStyle::DIALOG, "---");
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setParent(this);

    //this->updateActionHint(brls::Key::X, "Cancel Scan");
    //this->registerAction("Cancel Scan", brls::Key::X, [scanprog]() {
    //    scanprog.end_thread = true;
    //    return true;
    //});
}

void ScanningPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    if (!go)
    {
        print_debug("Go");
        go      = true;
        scanner = new std::thread(&ScanningPage::thread_scan, this);
    }
    else
    {
        if (!from_cache)
        {
            int num     = file_count;
            int num_out = scanprog.prev_num_files;

            this->progressDisp->setProgress(num, num_out);
            this->progressDisp->frame(ctx);

            if (num < num_out)
                this->label->setText(std::to_string(num) + " / " + std::to_string(num_out) + " Files Scanned");
            else
                this->label->setText(std::to_string(num) + " Files Scanned");

            this->label->frame(ctx);
        }
        else
        {
            this->progressDisp->setProgress(1, 1);
            this->progressDisp->frame(ctx);

            this->label->setText("Loading from Cache");

            this->label->frame(ctx);
        }

        this->progressDisp->frame(ctx);
        this->label->frame(ctx);

        if (!continued && scanprog.complete)
        {
            continued = true;
            scanner->join();

            print_debug("rm lock.");

            if (fs::exists(get_config_path() + "lock"))
                remove((get_config_path() + "lock").c_str());

            settings_set_value("scan", "settings changed", "false");

            brls::Application::pushView(new AppsListPage());

            scanprog.prev_num_files = file_count;
        }
    }
}

void ScanningPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    this->label->setWidth(roundf((float)this->width * style->CrashFrame.labelWidth));
    this->label->invalidate(true);

    this->label->setBoundaries(
        this->x + this->width / 2 - this->label->getWidth() / 2,
        this->y + (this->height - style->AppletFrame.footerHeight) / 2,
        this->label->getWidth(),
        this->label->getHeight());

    this->progressDisp->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth,
        this->y + this->height / 2,
        style->CrashFrame.buttonWidth * 2,
        style->CrashFrame.buttonHeight);
}

void ScanningPage::willAppear(bool resetState)
{
    this->progressDisp->willAppear(resetState);
}

void ScanningPage::willDisappear(bool resetState)
{
    this->progressDisp->willDisappear(resetState);
}

ScanningPage::~ScanningPage()
{
    print_debug("Scan Page End");

    scanprog.end_thread = true;

    if (go && scanner->joinable())
    {
        print_debug("Joinable");
        scanner->join();
    }

    delete this->progressDisp;
    delete this->label;
}
