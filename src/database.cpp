#include <endian.h>
#include "database.h"
#include <cstring>
std::pair<int, bool> getPrimaryKeyIndex(std::string);

uint16_t SQL_LITE::Database::getPageSize() { return pageSize; }
void SQL_LITE::Database::setPageSize(uint16_t size) { pageSize = be16toh(size); }

void SQL_LITE::Database::setRootPageTables(uint16_t tablesCount) { rootPageTables = tablesCount; }

void SQL_LITE::Database::getDbInfo()
{
    std::cout << "database page size: " << htole16(pageSize) << std::endl;
    std::cout << "number of tables: " << htole16(rootPageTables) << std::endl;
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
    {

        auto v = getPrimaryKeyIndex(sqlText);
        tableRootPagesMap.insert({tableName, std::make_tuple(rootPage, sqlText, v)});
    }
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
        return std::get<0>(tableRootPagesMap[tableName]);
    }

    return -1;
}

std::string SQL_LITE::Database::getRootPageCreateTableStatement(std::string tableName)
{
    if (tableRootPagesMap.find(tableName) != tableRootPagesMap.end())
    {
        return std::get<1>(tableRootPagesMap[tableName]);
    }

    return "";
}

std::pair<int, bool> SQL_LITE::Database::getPrimaryKeyInfoFromRootPage()
{
    if (parser == NULL)
        return std::make_pair(-1, false);
    if (tableRootPagesMap.find(parser->getFromClause()) != tableRootPagesMap.end())
    {
        return std::get<2>(tableRootPagesMap[parser->getFromClause()]);
    }
    return std::make_pair(-1, false);
}

std::pair<int, bool> getPrimaryKeyIndex(std::string sqlText)
{

    size_t start = 0;
    size_t end = 0;
    int counter = 0;
    while ((end = sqlText.find(',', start)) != std::string::npos)
    {
        std::string token = sqlText.substr(start, end - start);
        if (strstr(&token[0], "integer primary key"))
        {
            return std::make_pair(counter, true);
        }

        if (strstr(&token[0], "primary key"))
        {
            return std::make_pair(counter, false);
        }

        start = end + 1;
        counter++;
    }

    std::string token = sqlText.substr(start);

    if (strstr(&token[0], "integer primary key"))
    {
        return std::make_pair(counter, true);
    }

    if (strstr(&token[0], "primary key"))
    {
        return std::make_pair(counter, false);
    }

    return std::make_pair(0, false);
}

void SQL_LITE::Database::parseIndexSQLText(std::string sqlText, uint64_t rootPage)
{
    std::vector<std::string> delimiters{"on", "("};

    int offset = sqlText.find(delimiters[0]);
    std::string text = sqlText.substr(offset + delimiters[0].length());

    auto start = text.find(delimiters[1]);
    std::string table = SQL_LITE::trimWhiteSpaceAround(text.substr(0, start));
    std::string columnName = SQL_LITE::trimWhiteSpaceAround(text.substr(start + 1, text.length() - start - 2));

    if (!indexRootPagesMap.contains(table))
        indexRootPagesMap[table] = std::make_tuple(columnName, rootPage, sqlText);
}
