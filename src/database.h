#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include "statementParser.h"

namespace SQL_LITE
{
    class Database
    {
        uint16_t pageSize;
        uint16_t pageTables;
        std::map<std::string, std::pair<uint64_t, std::string>> tableRootPagesMap{};
        std::vector<std::string> supported_commands{".dbinfo", ".tables", "count(*)"};
        SQL_LITE::SQL_St_Parser *parser = NULL;

    public:
        inline SQL_LITE::SQL_St_Parser *getParser() { return parser; }
        uint16_t getPageSize();
        void setPageSize(uint16_t);
        uint16_t getPageTables();
        void setPageTables(uint16_t);
        void getDbInfo();
        void displayTableNames();
        void addTableRootPage(std::string, uint64_t, std::string);
        void execute(std::string);
        long long getRootPageNumber(std::string tableName);
        std::string getRootPageCreateTableStatement(std::string tableName);
    };
};

#endif