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
    for (auto it = tableRootPagesMap.begin(); it != tableRootPagesMap.end(); it++)
    {
        std::cout << it->first << " ";
    }
    std::cout << std::endl;
}

void SQL_LITE::Database::addTableRootPage(std::string tableName, uint64_t rootPage, std::string sqlText)
{
    if (!tableRootPagesMap.contains(tableName))
        tableRootPagesMap.insert({tableName, std::make_pair(rootPage, sqlText)});
}

void SQL_LITE::Database::execute(std::string command)
{
    if (command == supported_commands[0])
        return getDbInfo();
    if (command == supported_commands[1])
        return displayTableNames();

    parser = new SQL_LITE::SQL_St_Parser(command);
}

long long SQL_LITE::Database::getRootPageNumber(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return tableRootPagesMap[tableName].first;
    }

    return -1;
}

std::string SQL_LITE::Database::getRootPageCreateTableStatement(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return tableRootPagesMap[tableName].second;
    }

    return "";
}
