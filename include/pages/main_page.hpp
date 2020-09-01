#pragma once

#include <utils/utilities.h>

#include <borealis.hpp>

enum info_page_type
{
    info_page_dl_intro,
    info_page_dl_done
};

class MainPage : public brls::TabFrame
{
  public:

    brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length);
    brls::ListItem* make_app_entry(app_entry* entry, bool is_appstore = false);

    void build_main_tabs();

    MainPage();
    ~MainPage();

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;

  private:
    brls::Label* battery_label;
    brls::Label* time_label;
    brls::Label* date_label;

    brls::List* appsList;
    brls::List* storeAppsList;
    brls::List* localAppsList;
};
