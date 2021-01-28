#include <utils/settings.h>

#include <pages/apps_list_page.hpp>
#include <pages/layout_select_page.hpp>
#include <pages/main_page.hpp>

#include "utils/utilities.h"

using namespace brls::i18n::literals;

LayoutSelectPage::LayoutSelectPage()
{
    // Create views
    this->firstButton = new brls::Button(brls::ButtonStyle::REGULAR);
    this->firstButton->setImage(get_resource_path() + "style_beta.png");
    this->firstButton->registerAction("Select", brls::Key::A, []() { brls::Application::pushView(new MainPage()); return 0; });
    this->addView(this->firstButton);

    this->secondButton = new brls::Button(brls::ButtonStyle::REGULAR);
    this->secondButton->setImage(get_resource_path() + "style_list.png");
    this->secondButton->registerAction("Select", brls::Key::A, []() { brls::Application::pushView(new AppsListPage()); return 0; });
    this->addView(this->secondButton);

    this->thirdButton = new brls::Button(brls::ButtonStyle::REGULAR);
    this->thirdButton->setImage(get_resource_path() + "style_flow.png");
    this->addView(this->thirdButton);

    // Populate custom navigation map
    this->navigationMap.add(
        this->firstButton,
        brls::FocusDirection::RIGHT,
        this->secondButton);

    this->navigationMap.add(
        this->secondButton,
        brls::FocusDirection::LEFT,
        this->firstButton);

    this->navigationMap.add(
        this->secondButton,
        brls::FocusDirection::RIGHT,
        this->thirdButton);

    this->navigationMap.add(
        this->thirdButton,
        brls::FocusDirection::LEFT,
        this->secondButton);
}

#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 300
#define PADDING 200

void LayoutSelectPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    // Fully custom layout
    this->firstButton->setBoundaries(
        this->x + this->getWidth() / 2. - BUTTON_WIDTH / 2 - BUTTON_WIDTH / 2 - PADDING,
        this->y + this->getHeight() / 2. - BUTTON_HEIGHT / 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT);

    this->secondButton->setBoundaries(
        this->x + this->getWidth() / 2. - BUTTON_WIDTH / 2,
        this->y + this->getHeight() / 2. - BUTTON_HEIGHT / 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT);

    this->thirdButton->setBoundaries(
        this->x + this->getWidth() / 2. + BUTTON_WIDTH / 2 - BUTTON_WIDTH / 2 + PADDING,
        this->y + this->getHeight() / 2. - BUTTON_HEIGHT / 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT);
}

brls::View* LayoutSelectPage::getDefaultFocus()
{
    return this->firstButton->getDefaultFocus();
}

brls::View* LayoutSelectPage::getNextFocus(brls::FocusDirection direction, brls::View* currentView)
{
    return this->navigationMap.getNextFocus(direction, currentView);
}
