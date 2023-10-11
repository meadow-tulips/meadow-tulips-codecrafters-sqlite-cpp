#include <iostream>
#include "fileParser.h"

SQL_LITE::FileParser::FileParser(std::string fullFilePath) { filePath = fullFilePath; }

void SQL_LITE::FileParser::readFileAndExecuteCommand(std::string command)
{

    std::ifstream ifs{filePath, std::ios::binary};
    if (ifs.is_open())
    {
        ifs.seekg(16, std::ios_base::beg);

        if (command == supported_commands[0])
        {
            char buffer[2];
            short pageSize;
            ifs.read((char *)&pageSize, 2);
            // unsigned short page_size = (static_cast<unsigned char>(buffer[1]) | (static_cast<unsigned char>(buffer[0]) << 8));
            std::cout << "database page size: " << pageSize * 256 << std::endl;
        }
    }
}