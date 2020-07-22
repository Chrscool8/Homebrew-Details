#pragma once

#include <borealis.hpp>

struct app_entry
{
    std::string name;
    std::string file_name;
    std::string full_path;
    std::string author;
    std::string version;
    std::size_t size;
    std::size_t icon_size;
    uint8_t* icon;
    bool from_appstore;
    std::string url;
    std::string category;
    std::string license;
    std::string description;
    std::string summary;
    std::string changelog;
};

std::string get_online_version();
bool is_number(const std::string& s);

class MainPage : public brls::TabFrame
{
  public:
    std::vector<app_entry> local_apps;
    std::vector<app_entry> store_apps;
    std::vector<app_entry> store_file_data;

    void read_store_apps();
    void process_app_file(std::string filename);
    void load_all_apps();
    brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to);
    brls::ListItem* make_app_entry(app_entry* entry);

    MainPage();
    ~MainPage();

  private:
};
