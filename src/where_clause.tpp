#ifndef WHERE_CLAUSE_TPP
#define WHERE_CLAUSE_TPP
#include <iostream>

template <typename T>
SQL_LITE::WhereClause<T>::WhereClause(std::string str)
{
    char delimiter = ' ';
    int start = 0;
    int end = 0;
    for (int i = 0; i < 2 && ((end = str.find(delimiter, start)) != std::string::npos); i++)
    {
        std::string token = str.substr(start, end - start);
        if (i == 0)
            key = token;
        else if (i == 1 && token.length() == 1)
        {
            relation = token[0];
        }
        start = end + 1;
    }

    std::string valueToken = str.substr(start);

    if (valueToken.length() > 2 && (uint8_t)valueToken[0] == 39 && (uint8_t)valueToken[valueToken.length() - 1] == 39)
        value = valueToken.substr(1, valueToken.length() - 2);
    else
        value = valueToken;
}

template <typename T>
bool SQL_LITE::WhereClause<T>::isTrue(T val)
{
    if (relation == '=')
        return val == value;
    else if (relation == '>')
        return val > value;
    else if (relation == '<')
        return val < value;
    else
        return false;
}

#endif // WHERE_CLAUSE_TPP
