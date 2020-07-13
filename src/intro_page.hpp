#pragma once

#include <borealis.hpp>

class IntroPage : public brls::View
{
  private:
    brls::Button* button;
    brls::Label* label;
    brls::Image* image;

  public:
    IntroPage(std::string label);
    ~IntroPage();

    bool go;
    int short_wait;

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;
    brls::View* getDefaultFocus() override;
};
