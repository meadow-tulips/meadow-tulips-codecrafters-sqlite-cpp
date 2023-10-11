#include <cstring>
#include <iostream>
#include <fstream>
#include "fileParser.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    SQL_LITE::FileParser fs{argv[1]};

    fs.readFileAndExecuteCommand(argv[2]);

    return 0;
}
