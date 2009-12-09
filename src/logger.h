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

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#ifndef NOCOLOR
    #define COLOR_END "\033[0m"
    #define COLOR_WHITE "\033[1;37m"
    #define COLOR_YELLOW "\033[1;33m"
    #define COLOR_GREEN "\033[0;32m"
    #define COLOR_RED "\033[0;31m"
#else
    #define COLOR_END ""
    #define COLOR_WHITE ""
    #define COLOR_YELLOW ""
    #define COLOR_GREEN ""
    #define COLOR_RED ""
#endif

class BaseLogger
{
public:
    BaseLogger(std::ostream& output, const char* tag) : m_stream(output), m_tag(tag) {}
    ~BaseLogger()
    {
        m_stream << std::endl;
    }
    std::ostream& operator()() { return m_stream; };
    template <typename T>
    std::ostream& operator<<(const T& t) { return m_stream << m_tag << " :: " << t; }
private:
    std::ostream& m_stream;
    const char* m_tag;
};

class Warn : public BaseLogger
{
public:
    Warn() : BaseLogger(std::cout, COLOR_YELLOW "WARNING" COLOR_END) {}
};

struct MeiqueError
{
    MeiqueError();
    static bool errorAlreadyset;
};

class Error : public BaseLogger
{
public:
    Error() : BaseLogger(std::cerr, COLOR_RED "ERROR" COLOR_END) {}
    ~Error()
    {
        if (!MeiqueError::errorAlreadyset)
            throw MeiqueError();
    }
};

class Notice : public BaseLogger
{
public:
    Notice() : BaseLogger(std::cout, COLOR_GREEN "NOTICE" COLOR_END) {}
};

#endif // LOGGER_H
