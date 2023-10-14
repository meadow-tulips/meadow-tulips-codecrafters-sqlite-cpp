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

void SQL_LITE::Database::addTableRootPage(std::string tableName, uint64_t rootPage)
{
    tableRootPagesMap.insert({tableName, rootPage});
}

void SQL_LITE::Database::execute(std::string command)
{
    if (command == supported_commands[0])
        getDbInfo();
    else if (command == supported_commands[1])
        displayTableNames();

    else if (command.find("select ") == 0)
        executeSelect(command);
}

long long SQL_LITE::Database::executeSelect(std::string command)
{
    std::string whatEntity;
    std::string fromEntity;
    std::string delimiter = " ";
    std::vector<std::string> tokens{};

    size_t start = 0;
    size_t end = 0;

    while ((end = command.find(delimiter, start)) != std::string::npos)
    {

        tokens.push_back(command.substr(start, end - start));
        start = end + delimiter.length();
    }

    tokens.push_back(command.substr(start));
    // for (int i = 0; i < tokens.size(); i++)
    // {
    //     std::cout << tokens[i] << std::endl;
    // }

    if (tokens.size() < 4)
        return -1;
    if (tokens[1] == "count(*)")
        return getTotalRecordsFromTable(tokens[3]);
    return -1;
}

long long SQL_LITE::Database::getTotalRecordsFromTable(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return tableRootPagesMap[tableName];
    }

    return -1;
}