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
#include <string>
#include <thread>
#include <vector>

#include "switch/services/psm.h"

myprogress prog;

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    return written;
}

int progress_func(void* p, long long dltotal, long long dlnow, long long ultotal, long long ulnow)
{
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

    return 0;
}

bool UpdatingPage::download_update()
{
    print_debug("update time");

    CURL* curl_handle;
    static const char* pagefilename = "sdmc:/config/homebrew_details/hbupdate.nro";

    remove(pagefilename);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init the curl session */
    curl_handle = curl_easy_init();

    if (curl_handle)
    {
        prog.lastruntime = 0;
        prog.progress    = 0;
        prog.curl        = curl_handle;
        prog.complete    = false;
        prog.success     = false;
        prog.downloading = false;

        std::string url = "https://github.com/Chrscool8/Homebrew-Details/releases/download/v" + get_online_version_number() + "/homebrew_details.nro";

        print_debug(url);

        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Homebrew-Details");
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L); //only for https
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L); //only for https

        curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_func);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, &prog);

        FILE* pagefile = fopen(pagefilename, "wb");
        if (pagefile)
        {
            print_debug("pagefile good");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

            prog.downloading = true;
            CURLcode res     = curl_easy_perform(curl_handle);
            curl_easy_cleanup(curl_handle);
            fclose(pagefile);
            prog.downloading = false;
            prog.complete    = true;

            print_debug(std::string("res ") + std::to_string(res));
            if (res == CURLE_OK)
            {
                print_debug("curl update okay");
                if (fs::exists(pagefilename))
                {
                    print_debug("new version downloaded");

                    app_entry check;
                    read_nacp_from_file(pagefilename, &check);
                    if (check.name == "Homebrew Details")
                    {
                        print_debug("good nacp");

                        romfsExit();
                        remove(get_setting(setting_nro_path).c_str());
                        rename(pagefilename, get_setting(setting_nro_path).c_str());

                        finished_download = true;
                        prog.success      = true;
                        return true;
                    }
                    else
                        print_debug("bad nacp");
                }
            }
        }
    }

    finished_download = true;
    prog.success      = false;
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
