#pragma once

#include <utils/utilities.h>

#include <borealis.hpp>

std::string get_online_version();
bool is_number(const std::string& s);

class MainPage : public brls::TabFrame
{
  public:
    std::vector<std::string> favorites;
    std::vector<app_entry> local_apps;
    std::vector<app_entry> store_apps;
    std::vector<app_entry> store_file_data;

    void read_favorites();
    void write_favorites();
    void add_favorite(std::string str);
    void remove_favorite(std::string str);
    void read_store_apps();
    void process_app_file(std::string filename);
    void load_all_apps();
    brls::ListItem* add_list_entry(std::string title, std::string short_info, std::string long_info, brls::List* add_to);
    brls::ListItem* make_app_entry(app_entry* entry);

    MainPage();
    ~MainPage();

  private:
};
