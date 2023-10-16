#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

namespace SQL_LITE
{
    class Database
    {
        uint16_t pageSize;
        uint16_t pageTables;
        std::map<std::string, std::pair<uint64_t, std::string>> tableRootPagesMap{};
        std::vector<std::string> supported_commands{".dbinfo", ".tables", "SELECT"};
        std::vector<std::string> sqlResult;

    public:
        uint16_t getPageSize();
        void setPageSize(uint16_t);
        uint16_t getPageTables();
        void setPageTables(uint16_t);
        void getDbInfo();
        void displayTableNames();
        void addTableRootPage(std::string, uint64_t, std::string);
        void execute(std::string);
        std::pair<std::string, std::string> *executeSelect(std::string command);
        long long getRootPageNumber(std::string tableName);
        std::string getSqlStatement(std::string tableName);
        void addSqlResult(std::string res);
        void displaySqlResult();
    };
}