#include <borealis.hpp>

class CustomLayoutTab : public brls::AbsoluteLayout
{
  public:
    CustomLayoutTab();

    brls::View* getDefaultFocus() override;
    brls::View* getNextFocus(brls::FocusDirection direction, brls::View* currentView) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash);

  private:
    brls::NavigationMap navigationMap;

    brls::Button* firstButton;
    brls::Button* secondButton;
};
