#include <math.h>
#include <utils/settings.h>

#include <pages/intro_page.hpp>
#include <pages/issue_page.hpp>
#include <pages/main_page.hpp>

IssuePage::IssuePage()
{
    this->setActionAvailable(brls::Key::B, false);

    go         = false;
    asked      = false;
    short_wait = 0;

    // Label
    this->button = (new brls::Button(brls::ButtonStyle::BORDERLESS))->setLabel("Back to Basics.")->setImage(get_resource_path("warning_arrows.png"));
    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this](View* view) {
        if (!go)
        {
            printf("Clicked scan\n");
            go = true;
        }
    });

    this->image = (new brls::Image(get_resource_path("warning.png")));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, "Safe Mode Engaged!\nLooks like HD didn't finish its last scan.\nSome settings have been reset.", true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);
}

void IssuePage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    this->image->frame(ctx);
    this->label->frame(ctx);
    this->button->frame(ctx);

    if (go && !asked)
    {
        if (short_wait < 5)
            short_wait += 1;
        else
        {
            settings_set_value(setting_search_subfolders, "false");
            settings_set_value(setting_search_root, "false");
            settings_set_value(setting_scan_full_card, "false");
            settings_set_value(setting_autoscan, "false");
            settings_set_value(setting_scan_settings_changed, "true");

            asked = true;
            brls::Application::pushView(new IntroPage());
            go         = false;
            asked      = false;
            short_wait = 0;
        }
    }
}

brls::View* IssuePage::getDefaultFocus()
{
    return this->button;
}

void IssuePage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    this->image->setWidth(256);
    this->image->setHeight(256);
    this->image->invalidate(true);
    this->image->setBoundaries(
        this->x + 230 + 43,
        this->y + (this->height / 2) - this->image->getHeight() / 2,
        this->image->getWidth(),
        this->image->getHeight());

    //

    this->label->setWidth(roundf((float)this->width * style->CrashFrame.labelWidth));
    this->label->invalidate(true);

    this->label->setBoundaries(
        this->x + this->width / 2 - this->label->getWidth() / 2 + 140 + 43,
        this->y + (this->height) / 2 - 125 + this->label->getHeight() / 2 - 30 - 10,
        this->label->getWidth(),
        this->label->getHeight());

    this->button->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth / 2 + 140 + 52,
        this->y + this->height / 2 + 100 - this->button->getHeight() - 60,
        style->CrashFrame.buttonWidth,
        style->CrashFrame.buttonHeight);
    this->button->invalidate();
}

IssuePage::~IssuePage()
{
    delete this->label;
    delete this->button;
    delete this->image;
}
