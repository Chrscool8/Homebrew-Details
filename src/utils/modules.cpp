#include <utils/modules.h>
#include <utils/utilities.h>

#include <borealis.hpp>

void draw_status(brls::View* view, int x, int y, int width, int height, brls::Style* style, brls::FrameContext* ctx)
{
    brls::Label* battery_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    battery_label->setHorizontalAlign(NVG_ALIGN_RIGHT);
    battery_label->setParent(view);

    brls::Label* time_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    time_label->setHorizontalAlign(NVG_ALIGN_LEFT);
    time_label->setParent(view);

    brls::Label* date_label = new brls::Label(brls::LabelStyle::DIALOG, "TestLabel", false);
    date_label->setHorizontalAlign(NVG_ALIGN_LEFT);
    date_label->setParent(view);

    battery_label->setFontSize(18);
    battery_label->setText("Battery" + get_battery_status() + ": " + std::to_string(get_battery_percent()) + "%");
    battery_label->invalidate(true);
    battery_label->setBoundaries(x + width - battery_label->getWidth() - 50, y + style->AppletFrame.headerHeightRegular * .5 + 14 + 4, battery_label->getWidth(), battery_label->getHeight());
    battery_label->invalidate(true);
    battery_label->frame(ctx);

    time_label->setFontSize(18);
    time_label->setText(get_time());
    time_label->invalidate(true);
    time_label->setBoundaries(x + width - time_label->getWidth() - 50, y + style->AppletFrame.headerHeightRegular * .5 - 14 + 4, time_label->getWidth(), time_label->getHeight());
    time_label->invalidate(true);
    time_label->frame(ctx);

    date_label->setFontSize(18);
    date_label->setText(get_date() + "   |");
    date_label->invalidate(true);
    date_label->setBoundaries(x + width - date_label->getWidth() - 145, y + style->AppletFrame.headerHeightRegular * .5 - 14 + 4, date_label->getWidth(), date_label->getHeight());
    date_label->invalidate(true);
    date_label->frame(ctx);

    delete time_label;
    delete battery_label;
    delete date_label;
}