/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include <regex.h>
#include <string>

// regex.h is a *very* common name to avoid useless conflicts, so we use meiqueregex.h
#ifndef MEIQUEREGEX_H
#define MEIQUEREGEX_H

class Regex
{
public:
    Regex(const char* pattern);
    ~Regex();
    bool match(const std::string& str, int pos = 0);
    std::pair<int, int> group(int n) const;
    std::string group(int n, const std::string& str) const;
    bool isValid() const;
private:
    regex_t m_regex;
    regmatch_t* m_matches;

    Regex(const Regex&) = delete;
};

#endif
