#include "update_page.hpp"

#include <math.h>
#include <sys/select.h>

#include <chrono>
#include <thread>

#include "intro_page.hpp"
#include "issue_page.hpp"
#include "main_page.hpp"
#include "settings.h"
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

#include "main_page.hpp"
#include "nacp_utils.h"
#include "switch/services/psm.h"

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

    double fractiondownloaded = (double)dlnow / (double)dltotal;

    myp->progress = (fractiondownloaded)*100.;
    print_debug(std::to_string(myp->progress) + "%\n");

    if (myp->end_thread)
        return 1;

    return 0;
}

bool UpdatePage::download_update()
{
    print_debug("update time\n");

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

        std::string url = "https://github.com/Chrscool8/Homebrew-Details/releases/download/v" + get_online_version() + "/homebrew_details.nro";

        print_debug(url + "\n");

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
            print_debug("pagefile good\n");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

            prog.downloading = true;
            CURLcode res     = curl_easy_perform(curl_handle);
            curl_easy_cleanup(curl_handle);
            fclose(pagefile);
            prog.downloading = false;
            prog.complete    = true;

            print_debug((std::string("res ") + std::to_string(res) + "\n").c_str());
            if (res == CURLE_OK)
            {
                print_debug("curl update okay\n");
                if (fs::exists(pagefilename))
                {
                    print_debug("new version downloaded\n");

                    app_entry check;
                    read_nacp_from_file(pagefilename, &check);
                    if (check.name == "Homebrew Details")
                    {
                        print_debug("good nacp\n");

                        romfsExit();
                        remove(get_setting(setting_nro_path).c_str());
                        rename(pagefilename, get_setting(setting_nro_path).c_str());

                        finished_download = true;
                        prog.success      = true;
                        return true;
                    }
                    else
                        print_debug("bad nacp\n");
                }
            }
        }
    }

    finished_download = true;
    prog.success      = false;
    return false;
}

void UpdatePage::thread_counter()
{
    print_debug("count go\n");
    bool updated = download_update();

    if (updated)
    {
        brls::Dialog* update_results_dialog         = new brls::Dialog("Your app has been updated!\nPlease restart to see changes.");
        brls::GenericEvent::Callback closeCallback1 = [update_results_dialog](brls::View* view) {
            update_results_dialog->close();
        };
        update_results_dialog->addButton("Okay.", closeCallback1);
        update_results_dialog->setCancelable(true);
        update_results_dialog->open();
    }
    else
    {
        brls::Dialog* update_results_dialog         = new brls::Dialog("There was a problem updating, sorry.");
        brls::GenericEvent::Callback closeCallback1 = [update_results_dialog](brls::View* view) {
            update_results_dialog->close();
        };
        update_results_dialog->addButton("Okay.", closeCallback1);
        update_results_dialog->setCancelable(true);
        update_results_dialog->open();
    }

    this->label->setText("Update Process Complete.");
    this->label->invalidate();

    this->button->setLabel("Close App.");
    this->button->invalidate();

    print_debug("thread end\n");
}

UpdatePage::UpdatePage()
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
    this->button = (new brls::Button(brls::ButtonStyle::BORDERLESS))->setLabel("Get Started.")->setImage(BOREALIS_ASSET("download.jpg"));
    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this](View* view) {
        if (!go)
        {
            print_debug("Clicked DL\n");
            go = true;
        }

        if (finished_download)
        {
            print_debug("Closing.\n");
            brls::Application::quit();
        }
    });

    this->image = (new brls::Image(BOREALIS_ASSET("download.jpg")));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, (std::string("Update Wizard Engaged.\nv") + get_setting(setting_local_version) + "  " + " \uE090 " + "  v" + get_online_version()).c_str(), true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);
}

void UpdatePage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    this->image->frame(ctx);
    this->label->frame(ctx);
    this->button->frame(ctx);

    std::string button_text = "";

    if (prog.downloading)
    {
        int progress_per = round(prog.progress);
        if (is_number(std::to_string(progress_per)))
            button_text = std::string("Progress: ") + std::to_string(progress_per) + "%";
    }
    else if (prog.complete)
    {
        button_text = "Close App.";
    }
    else
    {
        button_text = "Start Download";
    }

    this->button->setLabel(button_text.c_str());
    this->button->invalidate();

    if (go && !asked)
    {
        if (short_wait < 5)
            short_wait += 1;
        else
        {
            print_debug("update\n");

            //bool updated = download_update();

            prog.end_thread = false;
            counter         = new std::thread(&UpdatePage::thread_counter, this);

            asked = true;
            go    = false;
        }
    }
}

brls::View* UpdatePage::getDefaultFocus()
{
    return this->button;
}

void UpdatePage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    this->image->setWidth(256);
    this->image->setHeight(256);
    this->image->invalidate(true);
    this->image->setBoundaries(
        this->x + 230 + 43,
        this->y + (this->height / 2) - this->image->getHeight() / 2,
        this->image->getWidth(),
        this->image->getHeight());

    //

    this->label->setWidth(roundf((float)this->width * style->CrashFrame.labelWidth));
    this->label->invalidate(true);

    this->label->setBoundaries(
        this->x + this->width / 2 - this->label->getWidth() / 2 + 140 + 43,
        this->y + (this->height) / 2 - 125 + this->label->getHeight() / 2,
        this->label->getWidth(),
        this->label->getHeight());

    this->button->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth / 2 + 140 + 43,
        this->y + this->height / 2 + 100 - this->button->getHeight(),
        style->CrashFrame.buttonWidth,
        style->CrashFrame.buttonHeight);
    this->button->invalidate();
}

UpdatePage::~UpdatePage()
{
    prog.end_thread = true;
    counter->join();

    delete this->label;
    delete this->button;
    delete this->image;
}
