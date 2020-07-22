#pragma once
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include "switch/services/psm.h"

//
#include <sys/select.h>
//
#include <curl/curl.h>
#include <curl/easy.h>
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <borealis.hpp>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "intro_page.hpp"
#include "launching.h"
#include "reboot_to_payload.h"
#include "settings.h"
#include "update_page.hpp"
#include "main_page.hpp"

void read_nacp_from_file(std::string path, app_entry* current);
bool read_icon_from_file(std::string path, app_entry* current);