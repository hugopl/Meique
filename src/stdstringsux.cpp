/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdstringsux.h"
#include <sstream>

std::string& stringReplace(std::string& str, const std::string& substr, const std::string& replace)
{
    size_t n = 0;
    while (true) {
        n = str.find(substr, n);
        if (n == std::string::npos)
            break;
        str.replace(n, substr.length(), replace);
        n += substr.length()+1;
    }
    return str;
}

std::string escape(std::string str)
{
    return std::move(stringReplace(str, "\"", "\\\""));
}

void trim(std::string& str)
{
    size_t s = str.find_first_not_of(" \t\r\n");
    if (s != std::string::npos) {
        str.erase(0, s);
    } else {
        str.clear();
        return;
    }

    size_t e = str.find_last_not_of(" \t\r\n");
    if (e != std::string::npos)
        str.erase(e + 1);
}

StringList split(const std::string& str, char sep)
{
    std::istringstream s(str);
    std::string token;
    StringList result;
    while (std::getline(s, token, sep)) {
        if (!token.empty())
            result.push_back(token);
    }
    return result;
}

std::string join(const StringList& list, const std::string& sep)
{
    std::string output;
    StringList::const_iterator itBegin = list.begin();
    StringList::const_iterator itEnd = list.end();

    if (itBegin != itEnd) {
        output.append(*itBegin);
        ++itBegin;
        for (; itBegin != itEnd; ++itBegin) {
            output.append(sep);
            output.append(*itBegin);
        }
    }
    return output;
}

