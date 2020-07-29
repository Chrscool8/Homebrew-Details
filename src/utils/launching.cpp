#include <utils/launching.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <string>

#include "switch.h"

unsigned int launch_nro(std::string path, std::string args)
{
    print_debug(std::string("Launching: ") + path + "\n");
    print_debug(std::string("Args: ") + args + "\n");
    return envSetNextLoad(path.c_str(), args.c_str());
}