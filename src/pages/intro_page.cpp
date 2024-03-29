#include <math.h>
#include <utils/modules.h>
#include <utils/panels.h>
#include <utils/settings.h>

#include <fstream>
#include <pages/intro_page.hpp>
#include <pages/main_page.hpp>
#include <pages/scanning_page.hpp>

IntroPage::IntroPage()
{
    autoscanned       = false;
    go                = false;
    asked             = false;
    short_wait        = 0;
    autoscan_cooldown = 0;

    from_cache = std::filesystem::exists(get_apps_cache_file());

    this->button = new brls::Button(brls::ButtonStyle::BORDERLESS);
    this->button->setImage(get_resource_path() + "arrows_small.png");
    if (from_cache)
        this->button->setLabel("Load Cache");
    else
        this->button->setLabel("Begin Scan");

    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this](View* view) {
        if (!go)
        {
            printf("Clicked scan\n");
            go = true;
        }
    });

    this->image = (new brls::Image(get_resource_path() + "icon.png"));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, "Welcome to NEW Homebrew Details\nBy: Chris Bradel", true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);

    if (from_cache)
    {
        this->registerAction("Clear Cache", brls::Key::MINUS, [this]() {
            settings_set_value("internal", "invalidate cache", "true");
            remove(get_apps_cache_file().c_str());
            this->button->setLabel("Begin Scan");
            this->button->invalidate();
            return true;
        });
    }

    this->registerAction("What's New", brls::Key::L, [this]() {
        show_whatsnew_panel();
        return true;
    });

    this->registerAction("Settings", brls::Key::Y, [this]() {
        show_settings_panel();
        return true;
    });

    this->updateActionHint(brls::Key::B, "");
}

void IntroPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
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
            asked = true;

            brls::AppletFrame* scanning_page = show_framed(new ScanningPage());
            scanning_page->setIcon(get_resource_path() + "icon.png");
            scanning_page->setTitle("Scanning for Apps");

            go         = false;
            asked      = false;
            short_wait = 0;
            //this->button->setLabel("Scan Again");
            //this->button->invalidate();
        }
    }

    if (!go && !autoscanned && autoscan_cooldown > 5)
    {
        if (settings_get_value_true("scan", "autoscan"))
        {
            this->button->getClickEvent()->fire(this);
        }
        autoscanned = true;
    }

    if (autoscan_cooldown <= 5)
        autoscan_cooldown += 1;

    draw_status(this, x, y - 90, width, height, style, ctx);
}

brls::View* IntroPage::getDefaultFocus()
{
    return this->button;
}

void IntroPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
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
        this->y + (this->height) / 2 - 125 + this->label->getHeight() / 2 - 10 + 5,
        this->label->getWidth(),
        this->label->getHeight());

    this->button->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth / 2 + 140 + 43,
        this->y + this->height / 2 + 150 - this->button->getHeight() - 60,
        style->CrashFrame.buttonWidth,
        style->CrashFrame.buttonHeight);
    this->button->invalidate();
}

IntroPage::~IntroPage()
{
    delete this->label;
    delete this->button;
    delete this->image;
}
