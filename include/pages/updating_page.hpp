#pragma once

#include <math.h>
#include <sys/select.h>
#include <utils/nacp_utils.h>
#include <utils/settings.h>
#include <utils/update.h>

#include <chrono>
#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>
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
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>

#include "switch/services/psm.h"

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream);
int progress_func(void* p, long long dltotal, long long dlnow, long long ultotal, long long ulnow);

struct myprogress
{
    long long lastruntime;
    CURL* curl;
    bool downloading;
    double progress;
    bool complete;
    bool success;
    bool end_thread;
};

extern myprogress prog;

class UpdatingPage : public brls::View
{
  private:
    brls::StagedAppletFrame* frame;
    brls::ProgressDisplay* progressDisp;
    brls::Label* label;
    int progressValue = 0;

  public:
    std::thread* counter;

    bool go;
    int short_wait;
    bool asked;
    bool finished_download;

    bool download_update();
    void thread_counter();

    UpdatingPage(brls::StagedAppletFrame* frame);
    ~UpdatingPage();

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;

    void willAppear(bool resetState = false) override;
    void willDisappear(bool resetState = false) override;
};
