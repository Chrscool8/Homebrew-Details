#include "pages/test_grid_page.hpp"

#include "utils/utilities.h"

using namespace brls::i18n::literals;

CustomLayoutTab::CustomLayoutTab()
{
    // Create views
    this->firstButton = new brls::Button(brls::ButtonStyle::REGULAR);
    this->firstButton->setImage(get_resource_path("style_flow.png"));
    this->addView(this->firstButton);

    this->secondButton = new brls::Button(brls::ButtonStyle::REGULAR);
    this->secondButton->setImage(get_resource_path("style_list.png"));
    this->addView(this->secondButton);

    // Populate custom navigation map
    this->navigationMap.add(
        this->firstButton,
        brls::FocusDirection::RIGHT,
        this->secondButton);

    this->navigationMap.add(
        this->secondButton,
        brls::FocusDirection::LEFT,
        this->firstButton);
}

#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 300
#define PADDING 75

void CustomLayoutTab::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    // Fully custom layout
    this->firstButton->setBoundaries(
        this->x + this->getWidth() / 2. - BUTTON_WIDTH / 2 - BUTTON_WIDTH / 2 - PADDING,
        this->y + this->getHeight() / 2. - BUTTON_HEIGHT / 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT);

    this->secondButton->setBoundaries(
        this->x + this->getWidth() / 2. + BUTTON_WIDTH / 2 - BUTTON_WIDTH / 2 + PADDING,
        this->y + this->getHeight() / 2. - BUTTON_HEIGHT / 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT);
}

brls::View* CustomLayoutTab::getDefaultFocus()
{
    return this->firstButton->getDefaultFocus();
}

brls::View* CustomLayoutTab::getNextFocus(brls::FocusDirection direction, brls::View* currentView)
{
    return this->navigationMap.getNextFocus(direction, currentView);
}
