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
    tableRootPagesMap.insert({tableName, std::make_pair(rootPage, sqlText)});
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

std::pair<std::string, std::string> *SQL_LITE::Database::executeSelect(std::string command)
{
    std::string delimiter = " ";
    std::vector<std::string> tokens{};

    size_t start = 0;
    size_t end = 0;

    while ((end = command.find(delimiter, start)) != std::string::npos)
    {
        std::string token = command.substr(start, end - start);
        if (token != "select" && token != "from")
            tokens.push_back(token);
        start = end + delimiter.length();
    }

    tokens.push_back(command.substr(start));
    // for (int i = 0; i < tokens.size(); i++)
    // {
    //     std::cout << tokens[i] << std::endl;
    // }

    if (tokens.size() < 2)
        return NULL;
    else
    {
        std::string expression;
        for (auto it = tokens.begin(); it != (tokens.end() - 1); it++)
            expression = expression + (*it);

        return new std::pair<std::string, std::string>(expression, tokens[tokens.size() - 1]);
    }
}

long long SQL_LITE::Database::getRootPageNumber(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return tableRootPagesMap[tableName].first;
    }

    return -1;
}

std::string SQL_LITE::Database::getSqlStatement(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return tableRootPagesMap[tableName].second;
    }

    return "";
}

void SQL_LITE::Database::addSqlResult(std::string res)
{
    sqlResult.push_back(res);
}

void SQL_LITE::Database::displaySqlResult()
{
    for (int i = 0; i < sqlResult.size(); i++)
    {
        std::cout << sqlResult[i] << std::endl;
    }
}
