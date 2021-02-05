#pragma once
#include <atomic>
#include <string>

void new_read_store_apps();
void new_list_files(const char* basePath, bool recursive);
void new_load_all_apps();
brls::Image* get_image_from_nro(std::string filename);
brls::Image* load_image_cache(std::string filename);

extern std::atomic<int> file_count;
extern std::string last_file_name;

struct scanprogress
{
    bool scanning;
    double progress;
    bool complete;
    bool success;
    bool end_thread;
    int prev_num_files;
};

extern scanprogress scanprog;