#include <utils/launching.h>
#include <utils/settings.h>
#include <utils/utilities.h>

#include <string>

#ifdef __SWITCH__
#include <switch.h>
#endif // __SWITCH__

unsigned int launch_nro(std::string path, std::string args)
{
#ifdef __SWITCH__
    print_debug(std::string("Launching: ") + path + "\n");
    print_debug(std::string("Args: ") + args + "\n");
    return envSetNextLoad(path.c_str(), args.c_str());
#else
    print_debug(std::string("Launching: ") + path + "\n");
    print_debug(std::string("Args: ") + args + "\n");
#endif // __SWITCH__
}