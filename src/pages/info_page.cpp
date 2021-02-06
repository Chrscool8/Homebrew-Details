#include <math.h>
#include <utils/settings.h>

#include <fstream>
#include <pages/info_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

InfoPage::InfoPage(brls::StagedAppletFrame* frame, int type)
    : frame(frame)
{
    //this->setActionAvailable(brls::Key::B, false);
    //this->updateActionHint(brls::Key::B, "");

    page_type = type;

    pressed_button   = false;
    button_processed = false;
    short_wait       = 0;

    std::string button_label;
    std::string icon;
    std::string title;

    this->updateActionHint(brls::Key::B, "");

    switch (page_type)
    {
        case info_page_dl_intro:
            button_label = "Begin Download";
            icon         = get_resource_path() + "download.png";
            title        = std::string(" Update Wizard Engaged.\nv ") + APP_VERSION + " " + " " + symbol_rightarrow() + " " + " v " + get_online_version_number();
            break;
        case info_page_dl_done:
            button_label = "Proceed!";
            icon         = get_resource_path() + "download.png";
            title        = "Done";
            break;
        default:
            button_label = "Info";
            icon         = get_resource_path() + "icon.png";
            title        = "Info";
            break;
    }

    this->button = (new brls::Button(brls::ButtonStyle::BORDERLESS))->setLabel(button_label)->setImage(icon);
    this->button->setParent(this);
    this->button->getClickEvent()->subscribe([this, frame](View* view) {
        if (!pressed_button)
        {
            print_debug("Clicked scan");
            //this->frame->nextStage();

            //this->button->setLabel("Scanning...");
            //this->button->invalidate();
            pressed_button = true;
        }
    });

    this->image = (new brls::Image(icon));
    this->image->setParent(this);

    this->label = new brls::Label(brls::LabelStyle::DIALOG, title, true);
    this->label->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->label->setVerticalAlign(NVG_ALIGN_MIDDLE);
    this->label->setParent(this);
}

void InfoPage::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx)
{
    this->image->frame(ctx);
    this->label->frame(ctx);
    this->button->frame(ctx);

    if (page_type == info_page_dl_done)
    {
        if (prog.success)
        {
            label->setText("Your app has been updated!\nPlease restart to see changes.");
            button->setLabel("Restart App");
        }
        else
        {
            label->setText("There was a problem updating, sorry.");
            button->setLabel("Restart App");
        }

        label->invalidate();
        button->invalidate();
    }

    if (pressed_button && !button_processed)
    {
        if (short_wait < 5)
            short_wait += 1;
        else
        {
            button_processed = true;

            switch (page_type)
            {
                case info_page_dl_done:
                    launch_nro(settings_get_value("meta", "nro path"), "\"" + settings_get_value("meta", "nro path") + "\"");
                    romfsExit();
                    brls::Application::quit();
                    break;
                default:
                    if (!frame->isLastStage())
                    {
                        //frame->setActionAvailable(brls::Key::B, false);
                        frame->nextStage();
                    }
                    break;
            }
        }
    }
}

brls::View* InfoPage::getDefaultFocus()
{
    return this->button;
}

void InfoPage::layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash)
{
    this->image->setWidth(256);
    this->image->setHeight(256);
    this->image->invalidate(true);
    this->image->setBoundaries(
        this->x + 230 + 43,
        this->y + (this->height / 2) - this->image->getHeight() / 2,
        this->image->getWidth(),
        this->image->getHeight());

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

InfoPage::~InfoPage()
{
    print_debug("Info Page End");

    delete this->label;
    delete this->button;
    delete this->image;
}
