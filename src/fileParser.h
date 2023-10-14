#ifndef SQL_PARSER_H
#define SQL_PARSER_H
#include <string>
#include <fstream>

namespace SQL_LITE
{
    class FileParser
    {
        std::string filePath;

    public:
        FileParser(std::string);
        void readFileAndExecuteCommand(std::string);
    };
};

#endif