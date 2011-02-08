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
#include <list>
#include <map>
#include "os.h"

#ifndef NOCOLOR
    #define COLOR_END "\033[0m"
    #define COLOR_WHITE "\033[1;37m"
    #define COLOR_YELLOW "\033[1;33m"
    #define COLOR_GREEN "\033[0;32m"
    #define COLOR_RED "\033[0;31m"
    #define COLOR_BLUE "\033[1;34m"
    #define COLOR_MAGENTA "\033[1;35m"
#else
    #define COLOR_END ""
    #define COLOR_WHITE ""
    #define COLOR_YELLOW ""
    #define COLOR_GREEN ""
    #define COLOR_RED ""
    #define COLOR_BLUE ""
    #define COLOR_MAGENTA ""
#endif

// definied in config.cpp
extern int verboseMode;

struct green {};
struct red {};
struct yellow {};
struct magenta {};
struct white {};
struct blue {};
struct nocolor {};

std::ostream& operator<<(std::ostream& out, const green&);
std::ostream& operator<<(std::ostream& out, const red&);
std::ostream& operator<<(std::ostream& out, const yellow&);
std::ostream& operator<<(std::ostream& out, const nocolor&);
std::ostream& operator<<(std::ostream& out, const blue&);
std::ostream& operator<<(std::ostream& out, const magenta&);
std::ostream& operator<<(std::ostream& out, const white&);

class LogWriter : public std::ostream
{
public:
    enum Options
    {
        None = 0,
        ShowPid = 1,
        Quiet = 2
    };

    LogWriter(std::ostream& output, Options options = None) : m_stream(output), m_options(options) {}
    ~LogWriter()
    {
        if (m_options & Quiet)
            return;
        m_stream << nocolor() << std::endl;
    }
    std::ostream& operator()() { return m_stream; };
    template <typename T>
    std::ostream& operator<<(const T& t)
    {
        if (m_options & Quiet)
            return *this;
        return m_stream << t;
    }

protected:
    std::ostream& m_stream;
    unsigned int m_options;
};

template<typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& pair)
{
    return out << '(' << pair.first << ", " << pair.second << ')';
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, const std::list<T>& list)
{
    out << '[';
    if (list.size()) {
        typename std::list<T>::const_iterator it = list.begin();
        out << *it;
        ++it;
        for (; it != list.end(); ++it)
            out << ", " << *it;
    }
    out << ']';
    return out;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, const std::set<T>& set)
{
    out << '{';
    if (set.size()) {
        typename std::set<T>::const_iterator it = set.begin();
        out << *it;
        ++it;
        for (; it != set.end(); ++it)
            out << ", " << *it;
    }
    out << '}';
    return out;
}

template<typename K, typename V>
inline std::ostream& operator<<(std::ostream& out, const std::map<K, V>& map)
{
    out << '{';
    if (map.size()) {
        out << map.begin()->first << " -> " << map.begin()->second;
        typename std::map<K, V>::const_iterator it = ++map.begin();
        for (; it != map.end(); ++it)
            out << ", " << it->first << " -> " << it->second;
    }
    out << '}';
    return out;
}

class Warn : public LogWriter
{
public:
    Warn() : LogWriter(std::cout)
    {
        m_stream << COLOR_YELLOW "WARNING" COLOR_END " :: ";
    }
};

struct MeiqueError
{
    MeiqueError();
    static bool errorAlreadyset;
};

class Error : public LogWriter
{
public:
    Error() : LogWriter(std::cerr)
    {
        m_stream << COLOR_RED "ERROR" COLOR_END " :: ";
    }
    ~Error()
    {
        if (!MeiqueError::errorAlreadyset)
            throw MeiqueError();
    }
};

class Notice : public LogWriter
{
public:
    Notice() : LogWriter(std::cout) {}
};

class Debug : public LogWriter
{
public:
    Debug(int level = 1) : LogWriter(std::cout)
    {
        if (level > verboseMode)
            m_options |= Quiet;
        else
            m_stream << white() << "> " << nocolor();
    }
};

#endif // LOGGER_H
