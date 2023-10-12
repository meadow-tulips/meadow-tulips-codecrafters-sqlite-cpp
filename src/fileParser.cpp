#include <iostream>
#include <cstring>
#include <endian.h>
#include "fileParser.h"

unsigned int ToHex(char x)
{
    return (0xff & (unsigned int)x);
}
void getDbInfo(std::ifstream &ifs)
{
    char buffer[2]{'\0'};
    short pageSize;
    short btreeHeaderSize = 0;
    ifs.read((char *)&pageSize, 2);
    ifs.seekg(100, std::ios_base::beg);
    memset(buffer, '\0', 2);
    ifs.read(buffer, 1);

    if ((ToHex(buffer[0]) == 0x02) || (ToHex(buffer[0]) == 0x05))
    {
        // Interior table BTree page
        btreeHeaderSize = 12;
    }
    else if ((ToHex(buffer[0]) == 0x0A) || (ToHex(buffer[0]) == 0x0D))
    {
        // Leaf Index BTree page
        btreeHeaderSize = 8;
    }
    else
    {
        // Not a bTree
        return;
    }
    memset(buffer, '\0', 1);
    ifs.seekg(2, std::ios::cur);
    uint16_t countOfCells{0};
    ifs.read((char *)&countOfCells, 2);

    std::cout << "database page size: " << pageSize * 256 << std::endl;
    std::cout << "number of tables: " << be16toh(countOfCells) << std::endl;
}

SQL_LITE::FileParser::FileParser(std::string fullFilePath) { filePath = fullFilePath; }

void SQL_LITE::FileParser::readFileAndExecuteCommand(std::string command)
{

    std::ifstream ifs{filePath, std::ios::binary};
    if (ifs.is_open())
    {
        ifs.seekg(16, std::ios_base::beg);

        if (command == supported_commands[0])
        {
            getDbInfo(ifs);
        }

        ifs.close();
    }
}