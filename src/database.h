#include <iostream>
#include<vector>
#include<string>


namespace SQL_LITE {

    class Database {
        uint16_t pageSize;
        uint16_t pageTables;
        std::vector<std::string> tableNames;

        public:
            uint16_t getPageSize();
            void setPageSize(uint16_t);
            uint16_t getPageTables();
            void setPageTables(uint16_t);
            void getDbInfo();
            void displayTableNames();
            void addTableName(std::string);
    };
}