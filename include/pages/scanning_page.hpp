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

struct scanprogress
{
    bool scanning;
    double progress;
    bool complete;
    bool success;
    bool end_thread;
    int prev_num_files;
};

extern scanprogress scanprog;

class ScanningPage : public brls::View
{
  private:
    brls::StagedAppletFrame* frame;
    brls::ProgressDisplay* progressDisp;
    brls::Label* label;
    int progressValue = 0;
    bool continued;

  public:
    std::thread* scanner;

    bool go;
    int short_wait;
    bool asked;
    bool finished_download;

    void thread_scan();

    ScanningPage();
    ~ScanningPage();

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;

    void willAppear(bool resetState = false) override;
    void willDisappear(bool resetState = false) override;
};
