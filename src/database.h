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
        uint16_t rootPageTables;
        std::map<std::string, std::tuple<uint64_t, std::string, std::pair<int, bool>>> tableRootPagesMap{};
        std::vector<std::string> supported_commands{".dbinfo", ".tables", "count(*)"};
        SQL_LITE::SQL_St_Parser *parser = NULL;

    public:
        inline SQL_LITE::SQL_St_Parser *getParser() { return parser; }
        uint16_t getPageSize();
        void setPageSize(uint16_t);
        inline uint16_t getRootPageTables() { return rootPageTables; }
        void setRootPageTables(uint16_t);
        void getDbInfo();
        void displayTableNames();
        void addTableRootPage(std::string, uint64_t, std::string);
        void execute(std::string);
        long long getRootPageNumber(std::string tableName);
        std::string getRootPageCreateTableStatement(std::string tableName);
        std::pair<int, bool> getPrimaryKeyInfoFromRootPage();
    };
};

#endif