#pragma once
#include <atomic>
#include <string>

void read_store_apps();
void process_app_file(std::string filename);
void list_files(const char* basePath, bool recursive);
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