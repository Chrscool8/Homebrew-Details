#include <utils/launching.h>
#include <utils/nacp_utils.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>
//

#include <math.h>

#include <chrono>
#include <thread>

//
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SWITCH__
#include <switch.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "switch/services/psm.h"
#endif // __SWITCH__

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
#include <string>
#include <thread>
#include <vector>

#define CURL_STATICLIB
//#include <curl/curl.h>

#ifdef __SWITCH__
#include <curl/curl.h>
#endif
//#ifdef _WIN32
//#include <extern/curl_win/curl.h>
//#endif // __SWITCH__

myprogress prog;

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    return written;
}

int progress_func(void* p, long long dltotal, long long dlnow, long long ultotal, long long ulnow)
{
#ifdef __SWITCH__
    struct myprogress* myp = (struct myprogress*)p;
    CURL* curl             = myp->curl;
    long long curtime      = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &curtime);

    if ((curtime - myp->lastruntime) >= 3)
    {
        myp->lastruntime = curtime;
    }

    //print_debug(std::to_string(dlnow) + "::" + std::to_string(dltotal) + "\n");

    double fractiondownloaded = (double)dlnow / (double)dltotal;

    if (!isnan(fractiondownloaded) && dltotal > 1000)
    {
        myp->progress = (fractiondownloaded)*100.;
        //print_debug(std::to_string(myp->progress) + "%\n");
    }

    if (myp->end_thread)
        return 1;

#endif // __SWITCH__
    return 0;
}

bool UpdatingPage::download_update()
{
#ifdef __SWITCH__
    print_debug("update time");

    CURL* curl;
    std::string new_update_file = (get_config_path() + "hbupdate.nro").c_str();
    print_debug(new_update_file);
    remove(new_update_file.c_str());

    print_debug("curl globally initting\n");
    //curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl = curl_easy_init();

    if (curl)
    {
        print_debug("curl easily initted\n");

        prog.lastruntime = 0;
        prog.progress    = 0;
        prog.curl        = curl;
        prog.complete    = false;
        prog.success     = false;
        prog.downloading = false;

        std::string url = "https://github.com/Chrscool8/Homebrew-Details/releases/download/v" + get_online_version_number() + "/homebrew_details.nro";

        print_debug(url);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Homebrew-Details");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //only for https

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_func);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);

        FILE* pagefile = fopen(new_update_file.c_str(), "wb");
        if (pagefile)
        {
            print_debug("pagefile good");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);

            prog.downloading = true;
            CURLcode res     = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            fclose(pagefile);
            prog.downloading = false;
            prog.complete    = true;

            print_debug(std::string("res ") + std::to_string(res));
            print_debug(std::string("res ") + std::to_string(res));
            if (res == CURLE_OK)
            {
                print_debug("curl update okay");
                if (fs::exists(new_update_file))
                {
                    print_debug("new version downloaded");

                    nlohmann::json app_json = read_nacp_from_file(new_update_file);
                    if (string_contains(json_load_value_string(app_json, "author"), "Chris Bradel"))
                    {
                        print_debug("good nacp");

                        romfsExit();
                        remove(settings_get_value("meta", "nro path").c_str());
                        rename(new_update_file.c_str(), settings_get_value("meta", "nro path").c_str());

                        finished_download = true;
                        prog.success      = true;
                        return true;
                    }
                    else
                        print_debug("bad nacp");
                }
            }
        }
        else
            print_debug("pagefile BAD");
    }
    else
        print_debug("curl NOT easily initted\n");

    finished_download = true;
    prog.success      = false;
#endif
    return false;
}

void UpdatingPage::thread_counter()
{
    print_debug("count go");
    bool updated = download_update();

    print_debug("thread end");

    frame->nextStage();
}

UpdatingPage::UpdatingPage(brls::StagedAppletFrame* frame)
    : frame(frame)
{
    finished_download = false;

    go         = false;
    asked      = false;
    short_wait = 0;

    prog.lastruntime = 0;
    prog.progress    = 0;
    prog.complete    = false;
    prog.success     = false;
    prog.downloading = false;

    // Label
    this->progressDisp = new brls::ProgressDisplay();
    this->progressDisp->setProgress(0, 1);
    this->progressDisp->setParent(this);
    this->label = new brls::Label(brls::LabelStyle::DIALOG, "Downloading: Homebrew_Details.nro", true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setParent(this);
}

void UpdatingPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    if (!go)
    {
        go      = true;
        counter = new std::thread(&UpdatingPage::thread_counter, this);
    }

    this->progressDisp->setProgress(prog.progress, 100);
    this->progressDisp->frame(ctx);
    this->label->frame(ctx);
}

void UpdatingPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
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

void UpdatingPage::willAppear(bool resetState)
{
    this->progressDisp->willAppear(resetState);
}

void UpdatingPage::willDisappear(bool resetState)
{
    this->progressDisp->willDisappear(resetState);
}

UpdatingPage::~UpdatingPage()
{
    print_debug("Page End");

    if (go && counter->joinable())
    {
        print_debug("Joinable");
        prog.end_thread = true;
        counter->join();
    }
}
