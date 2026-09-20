//
// Created by wufan on 2026/9/20.
//
#include "fivecore/util/string_utils.h"


namespace fivecore::util
{
    std::string trim(const std::string& s)
    {
        auto start = s.begin();
        while (start != s.end() && std::isspace(static_cast<unsigned char>(*start)))
        {
            ++start;
        }

        auto end = s.end();
        do
        {
            --end;
        } while (std::distance(start,end) > 0 && std::isspace(static_cast<unsigned char>(*end)));

        return std::string(start,end+1);
    }
}