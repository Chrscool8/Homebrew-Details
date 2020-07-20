#include "launching.h"

#include <string>

#include "settings.h"
#include "switch.h"

unsigned int launch_nro(std::string path, std::string args)
{
    print_debug(path + "\n");
    return envSetNextLoad(path.c_str(), args.c_str());
}