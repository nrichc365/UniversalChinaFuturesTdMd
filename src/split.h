#ifndef _STRING_SPLIT_
#define _STRING_SPLIT_
#pragma once

#include <vector>
#include <string>

void split(const std::string &src, const std::string & separator, std::vector<std::string> & dest)
{
    std::string str = src;
    std::string substring;
    std::string::size_type start = 0, index;
    dest.clear();
    index = str.find_first_of(separator, start);
    do
    {
        if (index != std::string::npos)
        {
            substring = str.substr(start, index - start);
            dest.push_back(substring);
            start = index + separator.size();
            index = str.find(separator, start);
            if (start == std::string::npos) break;
        }
    } while (index != std::string::npos);

    //the last part
    substring = str.substr(start);
    dest.push_back(substring);
}

#endif // _STRING_SPLIT_
