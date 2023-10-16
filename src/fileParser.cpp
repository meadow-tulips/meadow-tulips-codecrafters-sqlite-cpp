#include <iostream>
#include <cstring>
#include <iomanip>
#include <endian.h>
#include <set>
#include <unordered_set>
#include "fileParser.h"
#include "database.h"

namespace SQL_LITE
{
    std::vector<int> parseSQLStatement(std::string statement, std::string entityToSelect);
}
unsigned int ToHex(char x)
{
    return (0xff & (unsigned int)x);
}

uint64_t readVarint(std::ifstream &ifs)
{
    uint64_t result = 0;
    uint8_t bytes_read = 0;

    while (true)
    {
        uint8_t current;
        ifs.read((char *)&current, 1);
        bytes_read++;

        if (bytes_read < 9)
        {
            result = (result << 7) | (current & 0x7f);
            if (current < 0x80)
            {
                break;
            }
        }
        else
        {
            result = (result << 8) | current;
            break;
        }
    }

    return result;
}

void readBtreeLeafCellForAppropriateColumns(std::ifstream &ifs, SQL_LITE::Database &db, std::vector<std::pair<std::string, long long int>> valuesVarints, std::vector<int> columnIndices)
{
    if (columnIndices.size() == 0)
        return;
    std::string res;
    auto readPos = ifs.tellg();
    for (int p = 0; p < columnIndices.size(); p++)
    {
        ifs.seekg(readPos);
        for (int i = 0; i < valuesVarints.size(); i++)
        {
            if (valuesVarints[i].first == "text")
            {
                char arr[valuesVarints[i].second + 1]{'\0'};
                ifs.read(arr, valuesVarints[i].second);
                if (columnIndices[p] == i)
                {
                    if (res.length() == 0)
                    {
                        res = arr;
                    }
                    else
                    {
                        res += "|";
                        res += arr;
                    }
                    break;
                }
            }
            else if (valuesVarints[i].first == "integer")
            {
                uint64_t num{0};
                ifs.read((char *)&num, valuesVarints[i].second);

                // rootPages.push_back(htole64(num));
            }
            else if (valuesVarints[i].first == "float")
            {
                long double num{};
                ifs.read((char *)&num, valuesVarints[i].second);
                // rootPages.push_back(htole64(num));
            }
        }
    }
    if (res.length() > 0)
    {
        db.addSqlResult(res);
    }
}

void readBtreeLeafCellForSchemaTable(std::ifstream &ifs, SQL_LITE::Database &db, std::vector<std::pair<std::string, long long int>> valuesVarints)
{
    std::vector<std::string> tableNames;
    std::vector<uint64_t> rootPages;
    std::vector<std::string> sqlTexts;
    for (int i = 0; i < valuesVarints.size(); i++)
    {

        if (valuesVarints[i].first == "text")
        {
            char arr[valuesVarints[i].second + 1]{'\0'};
            ifs.read(arr, valuesVarints[i].second);
            if (i == 2)
            {
                // Master table & column tbl_name
                tableNames.push_back(arr);
            }
            else if (i == 4)
            {
                sqlTexts.push_back(arr);
            }
        }
        else if (valuesVarints[i].first == "integer")
        {
            uint64_t num{0};
            ifs.read((char *)&num, valuesVarints[i].second);
            rootPages.push_back(htole64(num));
        }
    }

    for (int i = 0; i < tableNames.size(); i++)
    {
        db.addTableRootPage(tableNames[i], rootPages[i], sqlTexts[i]);
    }
}

void readBtreeLeafCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, std::vector<int> columnIndices)
{
    auto cell_payload_size = readVarint(ifs);
    std::vector<std::pair<std::string, long long>> payloadSizes;

    auto rowId = readVarint(ifs);

    auto headerStartPos = ifs.tellg();
    auto headerSize = readVarint(ifs);
    auto lastPosition = ifs.tellg();
    while (ifs.tellg() < (headerSize + headerStartPos))
    {
        auto data_typeVarint = readVarint(ifs);
        if (data_typeVarint == 0)
            payloadSizes.push_back(std::make_pair("null", 0));
        else if (data_typeVarint == 8 || data_typeVarint == 9)
            payloadSizes.push_back(std::make_pair("boolean", 0));
        if (data_typeVarint > 0 && data_typeVarint < 7)
        {
            auto textLength = data_typeVarint < 4 ? data_typeVarint : data_typeVarint == 5 ? 6
                                                                                           : 8;
            payloadSizes.push_back(std::make_pair("integer", textLength));
        }
        else if (data_typeVarint == 7)
            payloadSizes.push_back(std::make_pair("float", 8));
        else if (data_typeVarint >= 13 && data_typeVarint % 2 == 1)
        {
            // Text
            auto textLength = (data_typeVarint - 13) / 2;
            payloadSizes.push_back(std::make_pair("text", textLength));
        }
        else if (data_typeVarint >= 12 && data_typeVarint % 2 == 0)
        {
            // Blob object
            auto textLength = (data_typeVarint - 12) / 2;
            payloadSizes.push_back(std::make_pair("blob", textLength));
        }
    }

    if (pageNum == 1)
        readBtreeLeafCellForSchemaTable(ifs, db, payloadSizes);
    else
        readBtreeLeafCellForAppropriateColumns(ifs, db, payloadSizes, columnIndices);
}

void readCell(std::ifstream &ifs, uint16_t cellOffset, uint16_t pageType, SQL_LITE::Database &db, int pageNum, std::vector<int> columnIndices)
{
    ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
    ifs.seekg(cellOffset, std::ios_base::cur);
    // reading cell header
    if (pageType == 0x0D)
    {
        readBtreeLeafCell(ifs, db, pageNum, columnIndices);
    }

    // reading cell body
}

short isPageBtree(uint8_t pageType)
{

    if ((ToHex(pageType) == 0x02) || (ToHex(pageType) == 0x05))
        return 12;
    else if ((ToHex(pageType) == 0x0A) || (ToHex(pageType) == 0x0D))
        return 8;
    else
        return -1;
}

void readPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, std::vector<int> columnIndices)
{
    char buffer[2]{'\0'};
    ifs.read(buffer, 1);
    uint8_t pageType = buffer[0];
    short btreeHeaderSize = 0;
    if ((btreeHeaderSize = isPageBtree(pageType)) > -1)
    {
        memset(buffer, '\0', 1);
        ifs.seekg(2, std::ios::cur);
        uint16_t pageCells{0};
        ifs.read((char *)&pageCells, 2);
        db.setPageTables(pageCells);
        ifs.seekg(btreeHeaderSize - 5, std::ios::cur);
        // Now reading cell pointers.
        int counter = 0;
        while (counter < db.getPageTables())
        {
            int16_t cellLocation;
            ifs.read((char *)&cellLocation, 2);
            auto nextCellLocation = ifs.tellg();
            readCell(ifs, be16toh(cellLocation), pageType, db, pageNum, columnIndices);
            ifs.seekg(nextCellLocation, std::ios::beg);
            counter++;
        }
    }
}

void readRootPageHeader(std::ifstream &ifs, SQL_LITE::Database &db)
{
    ifs.seekg(16, std::ios_base::beg);
    uint16_t pageSize;
    ifs.read((char *)&pageSize, 2);
    db.setPageSize(pageSize);
    ifs.seekg(100, std::ios_base::beg);
}

void readRootPage(std::ifstream &ifs, SQL_LITE::Database &db)
{
    readRootPageHeader(ifs, db);
    readPage(ifs, db, 1, {});
}

SQL_LITE::FileParser::FileParser(std::string fullFilePath) { filePath = fullFilePath; }

void SQL_LITE::FileParser::readFileAndExecuteCommand(std::string command)
{
    SQL_LITE::Database db;
    std::ifstream ifs{filePath, std::ios::binary};
    if (ifs.is_open())
    {
        readRootPage(ifs, db);
        if (command.find("select") == 0)
        {
            auto entity = db.executeSelect(command);
            if (entity == NULL)
                return;
            long long pageNum = db.getRootPageNumber(entity->second);
            if (pageNum < 1)
                return;
            ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);

            if (entity->first == "count(*)")
            {
                readPage(ifs, db, pageNum, {});
                std::cout << db.getPageTables() << std::endl;
            }
            else
            {
                std::string sqlText = db.getSqlStatement(entity->second);
                if (sqlText.length() < 1)
                    return;
                std::vector<int> positions = SQL_LITE::parseSQLStatement(sqlText, entity->first);
                readPage(ifs, db, pageNum, positions);
                db.displaySqlResult();
            }

            delete entity;
        }
        else
            db.execute(command);
        ifs.close();
    }
}

std::vector<int> SQL_LITE::parseSQLStatement(std::string sqlStatement, std::string entityToSelect)
{
    std::vector<int> positions;
    std::string delimiter = ",";
    std::vector<std::string> entities;
    std::vector<std::string> splitSqlTokens;

    size_t start = 0;
    size_t end = 0;
    int counter = 0;

    while ((end = entityToSelect.find(delimiter, start)) != std::string::npos)
    {
        std::string token = entityToSelect.substr(start, end - start);
        entities.push_back(token);
        start = delimiter.length() + end;
    }

    entities.push_back(entityToSelect.substr(start));

    start = 0;
    end = 0;

    auto statementStartPos = sqlStatement.find("(");
    if (statementStartPos == std::string::npos)
        return positions;
    std::string statement = sqlStatement.substr(statementStartPos);
    while ((end = statement.find(delimiter, start)) != std::string::npos)
    {
        std::string token = statement.substr(start, end - start);
        splitSqlTokens.push_back(token);
        start = end + delimiter.length();
    }

    splitSqlTokens.push_back(statement.substr(start));

    for (auto v = entities.begin(); v != entities.end(); v++)
    {
        for (int i = 0; i < splitSqlTokens.size(); i++)
        {
            if (splitSqlTokens[i].find(*v) != std::string::npos)
            {
                positions.push_back(i);
                break;
            }
        }
    }
    return positions;
}