#pragma once
#include <borealis.hpp>

class AppsListPage : public brls::AppletFrame
{
  public:
    AppsListPage();
    ~AppsListPage();
    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
};