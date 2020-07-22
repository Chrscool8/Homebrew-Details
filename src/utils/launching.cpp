#include <utils/launching.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <string>

#include "switch.h"

unsigned int launch_nro(std::string path, std::string args)
{
    print_debug(path + "\n");
    return envSetNextLoad(path.c_str(), args.c_str());
}