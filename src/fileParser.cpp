#include <iostream>
#include <cstring>
#include <iomanip>
#include <endian.h>
#include <set>
#include <unordered_set>
#include "fileParser.h"
#include "database.h"

void readPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum);
void searchPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, uint32_t KeyToLocate = 0);

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

void readBtreeLeafCellForAppropriateColumns(std::ifstream &ifs, SQL_LITE::Database &db, std::vector<std::pair<std::string, long long int>> valuesVarints, uint64_t rowId)
{
    auto parser = db.getParser();
    std::string rootPageSQLText = db.getRootPageCreateTableStatement(parser->getFromClause());
    std::vector<unsigned int> columnIndices = parser->getColumnPositions(rootPageSQLText);
    auto selectClause = parser->getSelectClause();
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

                    if ((p == (columnIndices.size() - 1)) && (parser->getWhereClause().length() > 0))
                    {

                        auto where_clause_ob = parser->getWhereClauseObject();
                        // Indice to monitor for where condition
                        if (!where_clause_ob.isTrue(arr))
                        {
                            res = "";
                            return;
                        }
                    }
                    else
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
                    }
                    break;
                }
            }
            else if (valuesVarints[i].first == "integer")
            {
                uint64_t num{0};
                ifs.read((char *)&num, valuesVarints[i].second);

                if (columnIndices[p] == i)
                {

                    if (res.length() == 0)
                    {
                        res = std::to_string(num);
                    }
                    else
                    {
                        res += "|";
                        res += std::to_string(num);
                    }

                    break;
                }
            }
            else if (valuesVarints[i].first == "float")
            {
                long double num{};
                ifs.read((char *)&num, valuesVarints[i].second);
                if (columnIndices[p] == i)
                {

                    if (res.length() == 0)
                    {
                        res = std::to_string(num);
                    }
                    else
                    {
                        res += "|";
                        res += std::to_string(num);
                    }

                    break;
                }
            }
            else if (valuesVarints[i].first == "blob")
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
                    // }
                    break;
                }
            }
            else if (valuesVarints[i].first == "null")
            {

                auto primaryKeyInfo = db.getPrimaryKeyInfoFromRootPage();
                if (columnIndices[p] == i && primaryKeyInfo.first == i && primaryKeyInfo.second == true)
                {
                    if (res.length() == 0)
                    {
                        res = std::to_string(rowId);
                    }
                    else
                    {
                        res += "|";
                        res += std::to_string(rowId);
                    }
                }

                if (columnIndices[p] == i && (p == (columnIndices.size() - 1)) && (parser->getWhereClause().length() > 0))
                {
                    res = "";
                    return;
                }
            }
        }
    }
    if (res.length() > 0)
    {
        parser->addSqlResult(res);
    }
}

void readBtreeLeafCellForSchemaTable(std::ifstream &ifs, SQL_LITE::Database &db, std::vector<std::pair<std::string, long long int>> valuesVarints)
{
    std::vector<std::string> schemaTypes;
    std::vector<std::string> tableNames;
    std::vector<uint64_t> rootPages;
    std::vector<std::string> sqlTexts;
    for (int i = 0; i < valuesVarints.size(); i++)
    {

        if (valuesVarints[i].first == "text")
        {
            char arr[valuesVarints[i].second + 1]{'\0'};
            ifs.read(arr, valuesVarints[i].second);
            if (i == 0)
            {
                schemaTypes.push_back(arr);
            }
            else if (i == 1)
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
            uint8_t _rowId[8]{0};
            uint64_t num{0};

            ifs.read((char *)&_rowId[8 - valuesVarints[i].second], valuesVarints[i].second);
            num = *((uint64_t *)_rowId);
            rootPages.push_back(be64toh(num));
        }
    }

    for (int i = 0; i < tableNames.size(); i++)
    {
        if (schemaTypes[i] == "table")
            db.addTableRootPage(tableNames[i], rootPages[i], sqlTexts[i]);
        else if (schemaTypes[i] == "index")
            db.parseIndexSQLText(sqlTexts[i], rootPages[i]);
    }
}

void readBtreeInteriorCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    int pageNumber;
    auto lastLocation = ifs.tellg();
    ifs.read((char *)&pageNumber, 4);
    pageNumber = be32toh(pageNumber);
    auto rowId = readVarint(ifs);
    readPage(ifs, db, pageNumber);
}

void readBtreeLeafCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{

    auto cell_payload_size = readVarint(ifs);
    std::vector<std::pair<std::string, long long>> payloadSizes;

    auto rowId = readVarint(ifs);

    auto headerStartPos = ifs.tellg();
    auto headerSize = readVarint(ifs);
    while (ifs.tellg() < (headerSize + headerStartPos))
    {
        auto data_typeVarint = readVarint(ifs);
        if (data_typeVarint == 0)
            payloadSizes.push_back(std::make_pair("null", 0));
        else if (data_typeVarint == 8 || data_typeVarint == 9)
            payloadSizes.push_back(std::make_pair("boolean", 0));
        else if (data_typeVarint > 0 && data_typeVarint < 7)
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
        readBtreeLeafCellForAppropriateColumns(ifs, db, payloadSizes, rowId);
}

void readCell(std::ifstream &ifs, uint16_t cellOffset, SQL_LITE::Database &db, int pageNum, uint8_t pageType)
{
    ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
    ifs.seekg(cellOffset, std::ios_base::cur);
    // reading cell header
    if (pageType == 0x0D)
    {
        if (pageNum != 1)
            db.addRecordCount();
        readBtreeLeafCell(ifs, db, pageNum);
    }
    else if (pageType == 0x05)
    {
        readBtreeInteriorCell(ifs, db, pageNum);
    }
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

std::pair<int, uint8_t> readPageHeader(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
    if (pageNum == 1)
        ifs.seekg(100, std::ios::cur);
    uint8_t pageType;
    ifs.read((char *)&pageType, 1);
    short btreeHeaderSize{-1};
    if ((btreeHeaderSize = isPageBtree(pageType)) > -1)
    {
        ifs.seekg(2, std::ios::cur); // Freeblock 2 byte offset
        uint16_t pageCells{0};
        ifs.read((char *)&pageCells, 2);
        ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
        ifs.seekg(pageNum > 1 ? btreeHeaderSize : 100 + btreeHeaderSize, std::ios::cur);
        return std::make_pair(be16toh(pageCells), pageType);
    }

    return std::make_pair(0, 0);
}
void readPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    std::pair<int, uint8_t> pageHeaderInfo = readPageHeader(ifs, db, pageNum);
    if (pageNum == 1 && pageHeaderInfo.second != -1)
    {
        db.setRootPageTables(pageHeaderInfo.first);
    }
    // // Now reading cell pointers.
    int offset = 0;
    auto startPosition = ifs.tellg();
    while (offset < pageHeaderInfo.first)
    {
        ifs.seekg(startPosition, std::ios::beg);
        ifs.seekg(offset * 2, std::ios::cur);
        int16_t cellLocation{'\0'};
        ifs.read((char *)&cellLocation, 2);
        readCell(ifs, be16toh(cellLocation), db, pageNum, pageHeaderInfo.second);
        offset++;
    }

    if (pageHeaderInfo.second == 0x05)
    {
        ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
        ifs.seekg(8, std::ios::cur);
        int pageNumber;
        ifs.read((char *)&pageNumber, 4);
        pageNumber = be32toh(pageNumber);
        readPage(ifs, db, pageNumber);
    }
}

int searchLeafPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, uint32_t keyToLocate)
{
    readVarint(ifs);

    auto rowId = readVarint(ifs);
    if (rowId != keyToLocate)
    {
        return 0;
    }
    else
    {
        std::vector<std::pair<std::string, long long>> payloadSizes;
        auto headerStartPos = ifs.tellg();
        auto headerSize = readVarint(ifs);
        while (ifs.tellg() < (headerSize + headerStartPos))
        {
            auto data_typeVarint = readVarint(ifs);
            if (data_typeVarint == 0)
                payloadSizes.push_back(std::make_pair("null", 0));
            else if (data_typeVarint == 8 || data_typeVarint == 9)
                payloadSizes.push_back(std::make_pair("boolean", 0));
            else if (data_typeVarint > 0 && data_typeVarint < 7)
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

        auto parser = db.getParser();
        std::string rootPageSQLText = db.getRootPageCreateTableStatement(parser->getFromClause());
        std::vector<unsigned int> columnIndices = parser->getColumnPositions(rootPageSQLText);

        std::string res;
        auto readPos = ifs.tellg();

        for (int p = 0; p < (parser->getWhereClause().length() > 0 ? columnIndices.size() - 1 : columnIndices.size()); p++)
        {

            ifs.seekg(readPos);
            for (int i = 0; i < payloadSizes.size(); i++)
            {

                if (payloadSizes[i].first == "text")
                {
                    char arr[payloadSizes[i].second + 1]{'\0'};
                    ifs.read(arr, payloadSizes[i].second);
                    if (columnIndices[p] == i && p != columnIndices.size() && (parser->getWhereClause().length() > 0))
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
                else if (payloadSizes[i].first == "integer")
                {
                    uint64_t num{0};
                    ifs.read((char *)&num, payloadSizes[i].second);

                    if (columnIndices[p] == i)
                    {

                        if (res.length() == 0)
                        {
                            res = std::to_string(num);
                        }
                        else
                        {
                            res += "|";
                            res += std::to_string(num);
                        }

                        break;
                    }
                }
                else if (payloadSizes[i].first == "float")
                {
                    long double num{};
                    ifs.read((char *)&num, payloadSizes[i].second);
                    if (columnIndices[p] == i)
                    {

                        if (res.length() == 0)
                        {
                            res = std::to_string(num);
                        }
                        else
                        {
                            res += "|";
                            res += std::to_string(num);
                        }

                        break;
                    }
                }
                else if (payloadSizes[i].first == "blob")
                {
                    char arr[payloadSizes[i].second + 1]{'\0'};
                    ifs.read(arr, payloadSizes[i].second);

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
                else if (payloadSizes[i].first == "null")
                {

                    auto primaryKeyInfo = db.getPrimaryKeyInfoFromRootPage();
                    if (columnIndices[p] == i && primaryKeyInfo.first == i && primaryKeyInfo.second == true)
                    {
                        if (res.length() == 0)
                        {
                            res = std::to_string(rowId);
                        }
                        else
                        {
                            res += "|";
                            res += std::to_string(rowId);
                        }
                    }
                }
            }
        }
        if (res.length() > 0)
        {
            std::cout << res << std::endl;
        }

        return -1;
    }
}

int searchInteriorPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, uint32_t keyToLocate)
{
    int leftChildPointerPageNumber;
    ifs.read((char *)&leftChildPointerPageNumber, 4);
    leftChildPointerPageNumber = be32toh(leftChildPointerPageNumber);
    auto key = readVarint(ifs);
    if (keyToLocate > key)
    {
        return 1;
    }
    else
    {
        searchPage(ifs, db, leftChildPointerPageNumber, keyToLocate);
        return -1;
    }
}

int searchIndexLeafCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    auto payloadSize = readVarint(ifs);

    std::vector<std::pair<std::string, long long>> payloadSizes;
    auto headerStartPos = ifs.tellg();
    auto headerSize = readVarint(ifs);
    while (ifs.tellg() < (headerSize + headerStartPos))
    {
        auto data_typeVarint = readVarint(ifs);
        if (data_typeVarint == 0)
            payloadSizes.push_back(std::make_pair("null", 0));
        else if (data_typeVarint == 8 || data_typeVarint == 9)
            payloadSizes.push_back(std::make_pair("boolean", 0));
        else if (data_typeVarint > 0 && data_typeVarint < 7)
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

    std::string nodeKey;
    uint64_t rowId;
    for (int i = 0; i < payloadSizes.size(); i++)
    {
        if (payloadSizes[i].first == "text")
        {
            char value[payloadSizes[i].second + 1]{'\0'};
            ifs.read(value, payloadSizes[i].second);
            nodeKey += value;
        }
        else if (payloadSizes[i].first == "integer")
        {

            uint8_t _rowId[8]{0};
            ifs.read((char *)&_rowId[8 - payloadSizes[i].second], payloadSizes[i].second);
            rowId = *((uint64_t *)_rowId);
        }
    }

    auto where_ob = db.getParser()->getWhereClauseObject();
    if (nodeKey > where_ob.getValue())
    {
        return -1;
    }
    else if (nodeKey < where_ob.getValue())
    {
        return 1;
    }
    else
    {
        db.insertIndexRowId(rowId);
        return 0;
    }
}

int searchIndexInteriorCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    int leftChildPointerPageNumber;
    ifs.read((char *)&leftChildPointerPageNumber, 4);
    leftChildPointerPageNumber = be32toh(leftChildPointerPageNumber);
    auto payloadSize = readVarint(ifs);

    std::vector<std::pair<std::string, long long>> payloadSizes;
    auto headerStartPos = ifs.tellg();
    auto headerSize = readVarint(ifs);
    while (ifs.tellg() < (headerSize + headerStartPos))
    {
        auto data_typeVarint = readVarint(ifs);
        if (data_typeVarint == 0)
            payloadSizes.push_back(std::make_pair("null", 0));
        else if (data_typeVarint == 8 || data_typeVarint == 9)
            payloadSizes.push_back(std::make_pair("boolean", 0));
        else if (data_typeVarint > 0 && data_typeVarint < 7)
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

    std::string nodeKey;
    uint64_t rowId;
    for (int i = 0; i < payloadSizes.size(); i++)
    {
        if (payloadSizes[i].first == "text")
        {
            char value[payloadSizes[i].second + 1];
            ifs.read(value, payloadSizes[i].second);
            nodeKey += value;
        }
        else if (payloadSizes[i].first == "integer")
        {

            uint8_t _rowId[8]{0};
            ifs.read((char *)&_rowId[8 - payloadSizes[i].second], payloadSizes[i].second);
            rowId = *((uint64_t *)_rowId);
        }
    }

    auto where_ob = db.getParser()->getWhereClauseObject();
    if (nodeKey > where_ob.getValue())
    {
        searchPage(ifs, db, leftChildPointerPageNumber);
        return -1;
    }
    else if (nodeKey < where_ob.getValue())
    {
        return 1;
    }
    else
    {
        db.insertIndexRowId(rowId);
        searchPage(ifs, db, leftChildPointerPageNumber);

        return 0;
    }
}

int searchCell(std::ifstream &ifs, uint16_t cellOffset, SQL_LITE::Database &db, int pageNum, uint8_t pageType, uint32_t keyToLocate = 0)
{
    ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
    ifs.seekg(cellOffset, std::ios_base::cur);
    // reading cell header
    if (pageType == 0x0D)
    {
        return searchLeafPage(ifs, db, pageNum, keyToLocate);
    }
    else if (pageType == 0x05)
    {
        return searchInteriorPage(ifs, db, pageNum, keyToLocate);
    }
    else if (pageType == 0x0A)
    {
        return searchIndexLeafCell(ifs, db, pageNum);
    }
    else if (pageType == 0x02)
    {
        return searchIndexInteriorCell(ifs, db, pageNum);
    }

    return 0;
}
void searchPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum, uint32_t KeyToLocate)
{
    std::pair<int, uint8_t> pageHeaderInfo = readPageHeader(ifs, db, pageNum);
    if (pageNum == 1 && pageHeaderInfo.second != -1)
    {
        db.setRootPageTables(pageHeaderInfo.first);
    }
    // // Now reading cell pointers.
    int offset = 0;
    auto startPosition = ifs.tellg();
    while (offset < pageHeaderInfo.first)
    {
        ifs.seekg(startPosition, std::ios::beg);
        ifs.seekg(offset * 2, std::ios::cur);
        int16_t cellLocation{'\0'};
        ifs.read((char *)&cellLocation, 2);
        if (searchCell(ifs, be16toh(cellLocation), db, pageNum, pageHeaderInfo.second, KeyToLocate) < 0)
            break;

        offset++;
    }

    if (pageHeaderInfo.second == 0x05 || pageHeaderInfo.second == 0x02)
    {
        ifs.seekg(db.getPageSize() * (pageNum - 1), std::ios::beg);
        ifs.seekg(8, std::ios::cur);
        int pageNumber;
        ifs.read((char *)&pageNumber, 4);
        pageNumber = be32toh(pageNumber);
        searchPage(ifs, db, pageNumber, KeyToLocate);
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
    readPage(ifs, db, 1);
}

SQL_LITE::FileParser::FileParser(std::string fullFilePath) { filePath = fullFilePath; }

void SQL_LITE::FileParser::readFileAndExecuteCommand(std::string command)
{
    SQL_LITE::Database db;
    std::ifstream ifs{filePath, std::ios::binary};
    if (ifs.is_open())
    {
        readRootPage(ifs, db);
        db.execute(command);

        auto parser = db.getParser();
        auto pageNum = db.getRootPageNumber(parser->getFromClause());
        auto select_clauses = parser->getSelectClause();
        if (select_clauses.size() < 0 || pageNum < 1)
            return;

        // if (select_clauses[0] == "count(*)")
        // {
        //     std::pair<int, uint8_t> pageInfo = readPageHeader(ifs, db, pageNum);
        //     std::cout << pageInfo.first << std::endl;
        // }

        auto indexTable = db.getRootIndexTable();
        if (indexTable.contains(parser->getFromClause()) && parser->getWhereClauseObject().getKey() == get<0>(indexTable[parser->getFromClause()]))
        {
            searchPage(ifs, db, get<1>(indexTable[parser->getFromClause()]));
            auto indexedRowIds = db.getIndexedRowIds();
            for (int i = 0; i < indexedRowIds.size(); i++)
            {
                searchPage(ifs, db, pageNum, indexedRowIds[i]);
            }
        }
        else
        {
            readPage(ifs, db, pageNum);
            if (select_clauses[0] == "count(*)")
            {
                std::cout << db.getTotalRecordsCount() << std::endl;
            }
            else
            {
                parser->displaySqlResult();
            }
        }
    }

    ifs.close();
}