#include <math.h>
#include <utils/settings.h>

#include <fstream>
#include <pages/intro_page.hpp>
#include <pages/main_page.hpp>
#include <pages/scanning_page.hpp>

IntroPage::IntroPage(std::string label)
{
    this->setActionAvailable(brls::Key::B, false);

    autoscanned       = false;
    go                = false;
    asked             = false;
    short_wait        = 0;
    autoscan_cooldown = 0;

    this->button = (new brls::Button(brls::ButtonStyle::BORDERLESS))->setLabel(label)->setImage(get_resource_path("arrows_small.jpg"));
    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this](View* view) {
        if (!go)
        {
            printf("Clicked scan\n");

            std::ofstream outputFile("sdmc:/config/homebrew_details/lock");
            if (outputFile)
            {
                outputFile << "lock";
                outputFile.close();
            }

            //this->button->setLabel("Scanning...");
            //this->button->invalidate();
            go = true;
        }
    });

    this->image = (new brls::Image(get_resource_path("icon.jpg")));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, "Welcome to Homebrew Details\nBy: Chris Bradel", true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);
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
            brls::Application::pushView(new ScanningPage());
            go         = false;
            asked      = false;
            short_wait = 0;
            //this->button->setLabel("Scan Again");
            //this->button->invalidate();
        }
    }

    if (!go && !autoscanned && autoscan_cooldown > 5)
    {
        if (get_setting_true(setting_autoscan))
        {
            this->button->getClickEvent()->fire(this);
        }
        autoscanned = true;
    }

    if (autoscan_cooldown <= 5)
        autoscan_cooldown += 1;
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
        this->y + (this->height) / 2 - 125 + this->label->getHeight() / 2,
        this->label->getWidth(),
        this->label->getHeight());

    this->button->setBoundaries(
        this->x + this->width / 2 - style->CrashFrame.buttonWidth / 2 + 140 + 43,
        this->y + this->height / 2 + 100 - this->button->getHeight(),
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
