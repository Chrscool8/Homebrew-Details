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
#include <curl/curl.h>
#include <curl/easy.h>
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

    read_favorites();
    read_blacklist();
    load_notes();

    if (get_setting_true(setting_invalidate_cache))
    {
        set_setting(setting_invalidate_cache, "");
        remove(get_apps_cache_file().c_str());
        set_setting(setting_scan_settings_changed, "false");
    }

    if (std::filesystem::exists(get_apps_cache_file()))
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

    set_setting(setting_previous_num_files, std::to_string(file_count));
    scanprog.complete = true;
}

ScanningPage::ScanningPage()
{
    std::ofstream outputFile("sdmc:/config/homebrew_details/lock");
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

    scanprog.progress = 0;
    scanprog.complete = false;
    scanprog.success  = false;
    scanprog.scanning = false;

    scanprog.end_thread = false;

    print_debug(get_setting(setting_previous_num_files) + " previous files");

    if (is_number(get_setting(setting_previous_num_files)))
        scanprog.prev_num_files = std::stoi(get_setting(setting_previous_num_files));
    else
        scanprog.prev_num_files = 1;

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
        if (get_setting_true(setting_scan_settings_changed))
        {
            this->progressDisp->setProgress(0, 1);
            this->progressDisp->frame(ctx);
            this->label->setText("First Time Scan: " + std::to_string(file_count) + " Files Scanned So Far");
            this->label->frame(ctx);
        }
        else
        {
            this->progressDisp->setProgress(file_count, scanprog.prev_num_files);
            this->progressDisp->frame(ctx);
            this->label->setText(std::to_string(file_count) + " / " + get_setting(setting_previous_num_files) + " Scanned");
            this->label->frame(ctx);
        }

        this->progressDisp->frame(ctx);
        this->label->frame(ctx);

        if (!continued && scanprog.complete)
        {
            continued = true;
            scanner->join();

            //print_debug("----------- Launch Layout Page");
            //show_framed(new LayoutSelectPage())->setTitle("Choose a layout style");

            print_debug("rm lock.");

            if (fs::exists("sdmc:/config/homebrew_details/lock"))
                remove("sdmc:/config/homebrew_details/lock");

            set_setting(setting_scan_settings_changed, "false");

            //brls::Application::popView();
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
