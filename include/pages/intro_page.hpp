#pragma once
#include <string.h>

#include <borealis.hpp>
#include <string>

class IntroPage : public brls::View
{
  private:
    brls::Button* button;
    brls::Label* label;
    brls::Image* image;

  public:
    IntroPage();
    ~IntroPage();

    bool go;
    int short_wait;
    bool asked;
    bool autoscanned;
    int autoscan_cooldown;

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;
    brls::View* getDefaultFocus() override;
};
