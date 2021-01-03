#pragma once
#include <atomic>
#include <string>

void new_read_store_apps();
void read_store_apps();
void process_app_file(std::string filename);
void new_list_files(const char* basePath, bool recursive);
void list_files(const char* basePath, bool recursive);
void new_load_all_apps();
void load_all_apps();

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