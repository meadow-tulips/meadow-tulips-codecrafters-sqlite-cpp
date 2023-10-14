#ifndef SQL_PARSER_H
#define SQL_PARSER_H
#include <string>
#include <vector>
#include <fstream>

namespace SQL_LITE
{
    class FileParser
    {
        std::string filePath;
        std::vector<std::string> supported_commands{".dbinfo", ".tables"};

    public:
        FileParser(std::string);
        void readFileAndExecuteCommand(std::string);
    };
};

#endif