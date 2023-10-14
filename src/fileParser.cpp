#include <iostream>
#include <cstring>
#include <iomanip>
#include <endian.h>
#include "fileParser.h"
#include "database.h"

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

void readBtreeLeafCell(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    auto cell_payload_size = readVarint(ifs);
    std::vector<std::pair<std::string, long long>> payloadSizes;

    auto rowId = readVarint(ifs);

    auto headerStartPos = ifs.tellg();
    auto headerSize = readVarint(ifs);
    auto lastPosition = ifs.tellg();
    // ifs.seekg(lastPosition);
    while (ifs.tellg() < (headerSize + headerStartPos))
    {
        auto data_typeVarint = readVarint(ifs);
        if (data_typeVarint == 0)
        {
            // null data
            // payloadSizes.push_back(std::make_pair("null", 0));
        }
        if (data_typeVarint >= 13 && data_typeVarint % 2 == 1)
        {
            // Text
            auto textLength = (data_typeVarint - 13) / 2;
            payloadSizes.push_back(std::make_pair("text", textLength));
        }
        if (data_typeVarint >= 12 && data_typeVarint % 2 == 0)
        {
            // Blob object
            auto textLength = (data_typeVarint - 12) / 2;
            payloadSizes.push_back(std::make_pair("blob", textLength));
        }
    }

    for (int i = 0; i < payloadSizes.size(); i++)
    {
        if (payloadSizes[i].first == "text")
        {
            char arr[payloadSizes[i].second + 1]{'\0'};
            ifs.read(arr, payloadSizes[i].second);
            if (pageNum == 1 && i == 2)
            {
                // Master table & column tbl_name
                db.addTableName(arr);
            }
        }
    }
}

void readCell(std::ifstream &ifs, uint16_t cellOffset, uint16_t pageType, SQL_LITE::Database &db, int pageNum)
{
    ifs.seekg(cellOffset, std::ios_base::beg);
    // reading cell header
    if (pageType == 0x0D)
    {
        readBtreeLeafCell(ifs, db, pageNum);
    }

    // reading cell body
}

short isPageBtree(std::ifstream &ifs, uint8_t pageType)
{

    if ((ToHex(pageType) == 0x02) || (ToHex(pageType) == 0x05))
        return 12;
    else if ((ToHex(pageType) == 0x0A) || (ToHex(pageType) == 0x0D))
        return 8;
    else
        return -1;
}

void readPage(std::ifstream &ifs, SQL_LITE::Database &db, int pageNum)
{
    char buffer[2]{'\0'};
    ifs.read(buffer, 1);
    uint8_t pageType = buffer[0];
    short btreeHeaderSize = 0;

    if ((btreeHeaderSize = isPageBtree(ifs, pageType)) > -1)
    {
        memset(buffer, '\0', 1);
        ifs.seekg(2, std::ios::cur);
        uint16_t pageCells{0};
        ifs.read((char *)&pageCells, 2);
        db.setPageTables(pageCells);
        ifs.seekg(100 + btreeHeaderSize, std::ios::beg);
        // Now reading cell pointers.
        int counter = 0;

        while (counter < db.getPageTables())
        {
            int16_t cellLocation;
            ifs.read((char *)&cellLocation, 2);
            auto nextCellLocation = ifs.tellg();
            readCell(ifs, be16toh(cellLocation), pageType, db, pageNum);
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

        if (command == supported_commands[0])
            db.getDbInfo();
        else if (command == supported_commands[1])
            db.displayTableNames();

        ifs.close();
    }
}