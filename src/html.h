// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

namespace html
{

class Header
{
private:
    std::map<std::string, std::string> headers;
public:
    Header() = default;

    Header(const std::string &headerUnit)
    {
        const static std::string HeaderDelim = ":";

        std::stringstream sstream(headerUnit);
        std::string entry;

        // Acquire first header line
        std::getline(sstream, entry);
        std::stringstream streamhat(entry);
        std::string word;
        std::vector<std::string> hatKeys = {"version", "path", "method"};

        for(;hatKeys.size() and streamhat >> word;)
        {
            headers[hatKeys.back()] = word;
            hatKeys.pop_back();
        }

        while(std::getline(sstream, entry))
        {
            size_t delim = entry.find(HeaderDelim);
            if(delim != std::string::npos)
            {
                std::string key = entry.substr(0, delim);
                auto crsign = entry.find('\r');
                std::string value = entry.substr(delim + 1, crsign - delim - 1);

                const auto lastcutted = std::remove(value.begin(), value.end(), ' ');
                headers[key] = std::string(value.begin(), lastcutted);
            }
        }
    };

    std::string getValue(const std::string &header)
    {
        const auto value = this->headers.find(header);
        if(value == headers.end()) return "";
        else return (*value).second;
    };
};

};
