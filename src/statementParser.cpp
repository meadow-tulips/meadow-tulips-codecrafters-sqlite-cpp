#include "statementParser.h"
#include <unordered_set>

std::string SQL_LITE::trimWhiteSpaceAround(std::string s)
{
    int startSpaces = 0;
    int endSpaces = 0;
    for (int i = 0; s[i] == ' ' && i < s.length(); i++)
        ++startSpaces;
    for (int i = s.length() - 1; s[i] == ' ' && i > -1; i--)
        ++endSpaces;

    return s.substr(startSpaces, s.length() - (endSpaces + startSpaces));
}

SQL_LITE::SQL_St_Parser::SQL_St_Parser(std::string st)
{
    statement = st;
    std::vector<std::string> delimiters{"select", "from", "where"};
    std::vector<std::string> delimitersUpperCase{"SELECT", "FROM", "WHERE"};
    size_t start = 0;
    size_t end = 0;
    int delimitersFound = 0;
    bool isDelimiterUpper = false;
    for (int i = 0; i < delimiters.size(); i++)
    {
        end = st.find(isDelimiterUpper ? delimitersUpperCase[i] : delimiters[i], start);
        if (i == 0 && end == std::string::npos)
        {
            end = st.find(delimitersUpperCase[i], start);
            isDelimiterUpper = true;
        }
        if (end == std::string::npos)
            break;
        else if (i == 1)
            select_clause = trimWhiteSpaceAround(st.substr(start, end - start));
        else if (i == 2)
            from_clause = trimWhiteSpaceAround(st.substr(start, end - start));

        start = end + (isDelimiterUpper ? delimitersUpperCase[i].length() : delimiters[i].length());
        delimitersFound++;
    }

    if (delimitersFound == 2)
        from_clause = trimWhiteSpaceAround(st.substr(start));
    else if (delimitersFound == 3)
        where_clause = trimWhiteSpaceAround(st.substr(start));

    if (where_clause.length() > 0)
    {
        where_clause_ob = SQL_LITE::WhereClause<std::string>(where_clause);
    }

    // std::cerr << "select:" << select_clause << 'x' << std::endl;
    // std::cerr << "from:" << from_clause << 'x' << std::endl;
    // std::cerr << "where:" << where_clause << 'x' << std::endl;
}

std::vector<std::string> SQL_LITE::SQL_St_Parser::getSelectClause()
{
    std::string delimiter = ",";
    std::vector<std::string> selected_cols{};

    size_t start = 0;
    size_t end = 0;

    while ((end = select_clause.find(delimiter, start)) != std::string::npos)
    {
        std::string token = select_clause.substr(start, end - start);
        selected_cols.push_back(trimWhiteSpaceAround(token));
        start = end + delimiter.length();
    }

    selected_cols.push_back(trimWhiteSpaceAround(select_clause.substr(start)));

    return selected_cols;
}

std::string SQL_LITE::SQL_St_Parser::getWhereClause()
{
    return where_clause;
}

std::vector<unsigned int> SQL_LITE::SQL_St_Parser::getColumnPositions(std::string rootPageSqlStatement)

{
    size_t start = 0;
    size_t end = 0;

    std::vector<unsigned> positions;
    std::string delimiter = ",";
    std::vector<std::string> splitSqlTokens;

    auto statementStartPos = rootPageSqlStatement.find("(");
    if (statementStartPos == std::string::npos)
        return positions;
    std::string statement = rootPageSqlStatement.substr(statementStartPos);
    while ((end = statement.find(delimiter, start)) != std::string::npos)
    {
        std::string token = statement.substr(start, end - start);
        splitSqlTokens.push_back(token);
        start = end + delimiter.length();
    }

    splitSqlTokens.push_back(statement.substr(start));

    std::vector<std::string> entities = getSelectClause();

    if (where_clause_ob.getKey().length() > 0)
    {
        entities.push_back(where_clause_ob.getKey());
    }

    for (auto v = entities.begin(); v != entities.end(); v++)
    {
        for (int i = 0; i < splitSqlTokens.size(); i++)
        {
            if (splitSqlTokens[i].find(*v) != std::string::npos)
            {
                if (i == entities.size() - 1 && where_clause_ob.getKey().length() > 0)
                {

                    where_clause_ob.setPosition(i);
                    positions.push_back(i);
                    break;
                }
                else
                {
                    positions.push_back(i);
                    break;
                }
            }
        }
    }
    return positions;
}

void SQL_LITE::SQL_St_Parser::displaySqlResult()
{
    for (int i = 0; i < sqlResult.size(); i++)
    {
        std::cout << sqlResult[i] << std::endl;
    }
}