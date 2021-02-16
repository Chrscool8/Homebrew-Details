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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utils/launching.h>
#include <utils/reboot_to_payload.h>
#include <utils/settings.h>

#include <algorithm>
#include <array>
#include <borealis.hpp>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pages/intro_page.hpp>
#include <pages/main_page.hpp>
#include <string>
#include <vector>

nlohmann::json errored_entry(std::string path);
nlohmann::json read_nacp_from_file(std::string path);
bool read_icon_from_file(std::string path, app_entry* current);