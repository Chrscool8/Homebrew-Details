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
    std::vector<std::string> favorites;
    std::vector<std::string> blacklist;
    std::vector<app_entry> local_apps;
    std::vector<app_entry> store_apps;
    std::vector<app_entry> store_file_data;

    void read_favorites();
    void write_favorites();
    void add_favorite(std::string str);
    void remove_favorite(std::string str);
    void read_store_apps();
    void process_app_file(std::string filename);
    void list_files(const char* basePath, bool recursive);
    void load_all_apps();
    brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to, int clip_length);
    brls::ListItem* make_app_entry(app_entry* entry);

    void build_main_tabs();

    MainPage();
    ~MainPage();

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;

  private:
    brls::Label* battery_label;
    brls::Label* time_label;
    brls::Label* date_label;
};
