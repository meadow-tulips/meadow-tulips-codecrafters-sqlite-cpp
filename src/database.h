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
        std::map<std::string, std::tuple<std::string, uint64_t, std::string>> indexRootPagesMap{};
        std::vector<std::string> supported_commands{".dbinfo", ".tables", "count(*)"};
        SQL_LITE::SQL_St_Parser *parser = NULL;
        std::vector<uint64_t> indexedRowIds;
        uint64_t totalRecordsOfaTable{0};

    public:
        inline SQL_LITE::SQL_St_Parser *getParser() { return parser; }
        inline std::map<std::string, std::tuple<uint64_t, std::string, std::pair<int, bool>>> getSchemaTable() { return tableRootPagesMap; }
        inline std::map<std::string, std::tuple<std::string, uint64_t, std::string>> getRootIndexTable() { return indexRootPagesMap; }
        inline void insertIndexRowId(uint64_t rowid) { indexedRowIds.push_back(be64toh(rowid)); }
        inline std::vector<uint64_t> getIndexedRowIds() { return indexedRowIds; }
        inline uint64_t addRecordCount() { return ++totalRecordsOfaTable; }
        inline uint64_t getTotalRecordsCount() { return totalRecordsOfaTable; }
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
        void parseIndexSQLText(std::string, uint64_t);
    };
};

#endif