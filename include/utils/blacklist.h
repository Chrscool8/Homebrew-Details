#pragma once

#include <string>

void read_blacklist();
void write_blacklist();
void add_blacklist(std::string str);
void remove_blacklist(std::string str);
bool blacklist_contains(std::string str);