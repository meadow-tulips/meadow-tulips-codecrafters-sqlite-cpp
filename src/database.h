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
        std::map<std::string, uint64_t> tableRootPagesMap{};
        std::vector<std::string> supported_commands{".dbinfo", ".tables", "SELECT"};

    public:
        uint16_t getPageSize();
        void setPageSize(uint16_t);
        uint16_t getPageTables();
        void setPageTables(uint16_t);
        void getDbInfo();
        void displayTableNames();
        void addTableRootPage(std::string, uint64_t);
        void execute(std::string);
        long long executeSelect(std::string command);
        long long getTotalRecordsFromTable(std::string tableName);
    };
}