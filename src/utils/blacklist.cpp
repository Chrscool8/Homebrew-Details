#include <main.h>
#include <utils/base64.h>
#include <utils/blacklist.h>
#include <utils/utilities.h>

#include <fstream>
#include <string>

void read_blacklist()
{
    blacklist.clear();

    std::ifstream inputFile("sdmc:/config/homebrew_details/blacklist.txt");
    if (inputFile)
    {
        while (inputFile)
        {
            char line[513];
            inputFile.getline(line, 512);
            if (strlen(line) != 0)
                blacklist.push_back(base64_decode(line));
            print_debug(line);
        }
        inputFile.close();
    }
    else
        print_debug("Can't find blacklist file.");
}

void write_blacklist()
{
    create_directories("sdmc:/config/homebrew_details/");
    remove("sdmc:/config/homebrew_details/blacklist.txt");

    if (!blacklist.empty())
    {
        std::ofstream outputFile("sdmc:/config/homebrew_details/blacklist.txt");
        if (outputFile)
        {
            int index = 0;
            while (index < blacklist.size())
            {
                outputFile << (base64_encode(blacklist.at(index))).c_str();
                if (index != blacklist.size() - 1)
                    outputFile << std::endl;
                index += 1;
            }
            outputFile.close();
            print_debug("BL written");
        }
        else
            print_debug("Can't open blacklist.");
    }
    else
        print_debug("BL not written");
}

void add_blacklist(std::string str)
{
    print_debug("add blacklist: " + str);
    if (!vector_contains(blacklist, str))
    {
        blacklist.push_back(str);
        write_blacklist();
    }
}

void remove_blacklist(std::string str)
{
    print_debug("remove blacklist: " + str);
    if (vector_contains(blacklist, str))
    {
        blacklist.erase(remove(blacklist.begin(), blacklist.end(), str), blacklist.end());
        write_blacklist();
    }
}