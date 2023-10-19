#ifndef WHERE_CLAUSE_HPP
#define WHERE_CLAUSE_HPP

#include <string>

namespace SQL_LITE
{
    template <typename T>
    class WhereClause
    {
        char relation;
        T value;
        T key;
        unsigned int position;

    public:
        WhereClause(std::string str);
        inline T getValue() { return value; }
        inline T getKey() { return key; }
        inline char getRelation() { return relation; }
        inline unsigned int getPosition() { return position; }
        void setPosition(unsigned int p) { position = p; }
        bool isTrue(T);
    };
};

#include "where_clause.tpp"
#endif // WHERE_CLAUSE_HPP
