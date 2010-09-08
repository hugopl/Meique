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

#include "meiqueregex.h"
#include <cassert>

Regex::Regex(const char* pattern) : m_matches(0)
{
    int res = regcomp(&m_regex, pattern, REG_EXTENDED);
    if (!res)
        m_matches = new regmatch_t[m_regex.re_nsub + 1];
}

Regex::~Regex()
{
    delete[] m_matches;
    regfree(&m_regex);
}

bool Regex::isValid()
{
    return m_matches;
}

bool Regex::match(const std::string& str)
{
    return !regexec(&m_regex, str.c_str(), m_regex.re_nsub + 1, m_matches, 0);
}

std::pair<int, int> Regex::group(int n)
{
    std::size_t start = m_matches[n].rm_so;
    std::size_t end = m_matches[n].rm_eo - start;
    return std::pair<int, int>(start, end);
}

std::string Regex::group(int n, const std::string& str)
{
    std::size_t start = m_matches[n].rm_so;
    std::size_t end = m_matches[n].rm_eo - start;
    return str.substr(start, end);
}
