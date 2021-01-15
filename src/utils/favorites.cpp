#include <main.h>
#include <utils/base64.h>
#include <utils/favorites.h>
#include <utils/utilities.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void read_favorites()
{
    favorites.clear();

    std::ifstream inputFile(get_config_path("favorites.txt").c_str());
    if (inputFile)
    {
        while (inputFile)
        {
            char line[513];
            inputFile.getline(line, 512);
            if (strlen(line) != 0)
                favorites.push_back(base64_decode(line));
            print_debug(line);
        }
        inputFile.close();
    }
    else
        print_debug("Can't find favorites file.");
}

void write_favorites()
{
    create_directories(get_config_path(""));
    remove(get_config_path("favorites.txt").c_str());

    if (!favorites.empty())
    {
        std::ofstream outputFile(get_config_path("favorites.txt"));
        if (outputFile)
        {
            unsigned int index = 0;
            while (index < favorites.size())
            {
                if (!favorites.at(index).empty())
                {
                    outputFile << (base64_encode(favorites.at(index))).c_str();
                    if (index != favorites.size() - 1)
                        outputFile << std::endl;
                }
                index += 1;
            }
            outputFile.close();
        }
        else
            print_debug("Can't open favorites.");
    }
}

void add_favorite(std::string str)
{
    print_debug("add fav: " + str);
    favorites.push_back(str);
    write_favorites();
}

void remove_favorite(std::string str)
{
    print_debug("remove fav: " + str);

    favorites.erase(std::remove(favorites.begin(), favorites.end(), str), favorites.end());
    write_favorites();
}

bool is_favorite(std::string str)
{
    return vector_contains(favorites, str);
}