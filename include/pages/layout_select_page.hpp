#include <borealis.hpp>

class LayoutSelectPage : public brls::AbsoluteLayout
{
  public:
    LayoutSelectPage();

    brls::View* getDefaultFocus() override;
    brls::View* getNextFocus(brls::FocusDirection direction, brls::View* currentView) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash);

  private:
    brls::NavigationMap navigationMap;

    brls::Button* firstButton;
    brls::Button* secondButton;
    brls::Button* thirdButton;
};
