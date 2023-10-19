#ifndef STATEMENT_H
#define STATEMENT_H
#include <iostream>
#include <string>
#include <vector>
#include "where_clause.hpp"

namespace SQL_LITE
{
    class SQL_St_Parser
    {
        std::string statement{""};
        std::string where_clause{""};
        std::string select_clause{""};
        std::string from_clause{""};
        WhereClause<std::string> where_clause_ob{""};
        std::vector<std::string> sqlResult{};

    public:
        inline std::string getStatement()
        {
            return statement;
        }
        inline std::string getFromClause() { return from_clause; }

        SQL_St_Parser(std::string statement);
        std::vector<std::string> getSelectClause();
        std::string getWhereClause();
        inline WhereClause<std::string> getWhereClauseObject() { return where_clause_ob; }
        std::vector<unsigned int> getColumnPositions(std::string);
        void displaySqlResult();
        inline void addSqlResult(std::string res) { sqlResult.push_back(res); }
    };
};

#endif