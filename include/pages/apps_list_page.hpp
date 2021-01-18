#pragma once
#include <borealis.hpp>
#include <nlohmann/json.hpp>

class AppsListPage : public brls::AppletFrame
{
  public:
    AppsListPage();
    ~AppsListPage();
    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;

    brls::ListItem* new_new_make_app_entry(nlohmann::json app_json);
    brls::ListItem* create_sort_type_choice(std::string label, std::string sort_name, std::string secondary_sort);
    brls::ListItem* create_sort_group_choice(std::string label, std::string sort_name);
    brls::List* build_app_list();
    void refresh_list();

    bool needs_refresh = false;
    bool do_refresh = false;
    unsigned int ticker = 0;
    brls::List* main_list;
};