#include <endian.h>
#include "database.h"

uint16_t SQL_LITE::Database::getPageSize() { return pageSize; }
void SQL_LITE::Database::setPageSize(uint16_t size) { pageSize = be16toh(size); }

uint16_t SQL_LITE::Database::getPageTables() { return pageTables; }
void SQL_LITE::Database::setPageTables(uint16_t tablesCount) { pageTables = be16toh(tablesCount); }

void SQL_LITE::Database::getDbInfo()
{
    std::cout << "database page size: " << htole16(pageSize) << std::endl;
    std::cout << "number of tables: " << htole16(pageTables) << std::endl;
}

void SQL_LITE::Database::displayTableNames()
{
    for (int i = 0; i < tableNames.size(); i++)
    {
        std::cout << tableNames[i] << " ";
    }
    std::cout << std::endl;
}

void SQL_LITE::Database::addTableName(std::string tableName)
{
    tableNames.push_back(tableName);
}