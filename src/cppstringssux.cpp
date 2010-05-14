/*
    This file is part of the Meique project
    Copyright (C) 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "cppstringssux.h"

void stringReplace(std::string& str, const std::string& substr, const std::string& replace) {
    size_t n = 0;
    while (true) {
        n = str.find(substr, n);
        if (n == std::string::npos)
            break;
        str.replace(n, substr.length(), replace);
        n += substr.length()+1;
    }
}

void trim(std::string& str)
{
    size_t e = str.find_last_not_of(" \t\r\n");
    if (e != std::string::npos)
        str.erase(e + 1);
    size_t s = str.find_first_not_of(" \t\r\n");
    if (s != std::string::npos)
        str.erase(0, s);
}

