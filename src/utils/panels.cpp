#include <utils/panels.h>
#include <utils/settings.h>
#include <utils/update.h>
#include <utils/utilities.h>

#include <borealis.hpp>
#include <pages/info_page.hpp>
#include <pages/intro_page.hpp>
#include <pages/main_page.hpp>
#include <pages/updating_page.hpp>

void show_update_panel()
{
    brls::TabFrame* appView = new brls::TabFrame();
    appView->sidebar->setWidth(1000);
    std::string vers = " v" + get_setting(setting_local_version) + "  " + " " + symbol_rightarrow() + " " + "  v" + get_online_version_number() + "\n\n";
    appView->sidebar->addView(new brls::Header("Update Actions", false));
    brls::ListItem* dialogItem = new brls::ListItem("Update Wizard");

    dialogItem->getClickEvent()->subscribe([&](brls::View* view) {
        brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
        stagedFrame->setTitle("Update Wizard");
        stagedFrame->setIcon(get_resource_path("icon.png"));
        stagedFrame->setActionAvailable(brls::Key::B, false);

        stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_intro));
        stagedFrame->addStage(new UpdatingPage(stagedFrame));
        stagedFrame->addStage(new InfoPage(stagedFrame, info_page_dl_done));

        brls::Application::pushView(stagedFrame);
    });

    print_debug(get_online_version_number());

    appView->sidebar->addView(dialogItem);
    appView->sidebar->addView(new brls::Label(brls::LabelStyle::REGULAR, " \n ", true));
    appView->sidebar->addView(new brls::Header("New Version Details", false));
    appView->sidebar->addView(add_list_entry("Online Version", "v" + get_online_version_number(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Title", get_online_version_name(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Description", get_online_version_description(), "", NULL, 40));
    appView->sidebar->addView(add_list_entry("Date", get_online_version_date(), "", NULL, 40));

    appView->setIcon(get_resource_path("download.png"));
    brls::PopupFrame::open("Update Info", appView, vers, "");
}