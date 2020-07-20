#include "update_page.hpp"

#include <math.h>

#include "intro_page.hpp"
#include "issue_page.hpp"
#include "main_page.hpp"
#include "settings.h"

UpdatePage::UpdatePage()
{
    //this->setActionAvailable(brls::Key::B, false);

    go         = false;
    asked      = false;
    short_wait = 0;

    // Label
    this->button = (new brls::Button(brls::ButtonStyle::BORDERLESS))->setLabel("Get Started.")->setImage(BOREALIS_ASSET("download.jpg"));
    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this](View* view) {
        if (!go)
        {
            if (!asked)
            {
                print_debug("Clicked DL\n");

                this->button->setLabel("Download in Progress");
                this->button->invalidate();
                go = true;
            }
            else
            {
                print_debug("Closing.\n");
                brls::Application::quit();
                //exit(0);
            }
        }
    });

    this->image = (new brls::Image(BOREALIS_ASSET("download.jpg")));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, (std::string("Update Wizard Engaged.\nv") + get_setting(setting_local_version) + "  " + " \uE090 " + "  v" + get_online_version()).c_str(), true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);
}

void UpdatePage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
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
            print_debug("update\n");

            bool updated = download_update();

            if (updated)
            {
                brls::Dialog* update_results_dialog         = new brls::Dialog("Your app has been updated!\nPlease restart to see changes.");
                brls::GenericEvent::Callback closeCallback1 = [update_results_dialog](brls::View* view) {
                    update_results_dialog->close();
                };
                update_results_dialog->addButton("Okay.", closeCallback1);
                update_results_dialog->setCancelable(true);
                update_results_dialog->open();
            }
            else
            {
                brls::Dialog* update_results_dialog         = new brls::Dialog("There was a problem updating, sorry.");
                brls::GenericEvent::Callback closeCallback1 = [update_results_dialog](brls::View* view) {
                    update_results_dialog->close();
                };
                update_results_dialog->addButton("Okay.", closeCallback1);
                update_results_dialog->setCancelable(true);
                update_results_dialog->open();
            }

            this->label->setText("Update Process Complete.");
            this->label->invalidate();

            this->button->setLabel("Close App.");
            this->button->invalidate();

            asked = true;
            go    = false;
            //brls::Application::popView();
            //brls::Application::pushView(new IntroPage("Begin Scan"));
            //go         = false;
            //asked      = false;
            //short_wait = 0;
        }
    }
}

brls::View* UpdatePage::getDefaultFocus()
{
    return this->button;
}

void UpdatePage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
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

UpdatePage::~UpdatePage()
{
    delete this->label;
    delete this->button;
    delete this->image;
}
