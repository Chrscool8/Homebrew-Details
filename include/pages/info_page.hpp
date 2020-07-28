#pragma once
#include <string.h>

#include <borealis.hpp>
#include <string>

class InfoPage : public brls::View
{
  private:
    brls::Button* button;
    brls::Label* label;
    brls::Image* image;
    brls::StagedAppletFrame* frame;

  public:
    InfoPage(brls::StagedAppletFrame* frame, int type);
    ~InfoPage();

    bool pressed_button;
    int short_wait;
    bool button_processed;
    bool autoscanned;
    int autoscan_cooldown;

    int page_type;

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;
    brls::View* getDefaultFocus() override;
};
