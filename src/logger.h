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

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <list>
#include <map>
#include <fstream>
#include "os.h"

extern int verbosityLevel;
extern bool coloredOutputEnabled;

enum Manipulators {
    NoColor,
    White,
    Yellow,
    Green,
    Red,
    Blue,
    Magenta,
    Cyan,
    NoBreak
};

class LogWriter
{
public:
    enum Options
    {
        None = 0,
        NoBreak = 1,
        Quiet = 2
    };

    LogWriter(std::ostream& output, unsigned int options = None) : m_stream(output), m_options(options) {}
    ~LogWriter();
    LogWriter(const LogWriter& other);

    template <typename T>
    LogWriter& operator<<(const T& t)
    {
        if (!(m_options & Quiet))
            m_stream << t;
        return *this;
    }

protected:
    std::ostream& m_stream;
    unsigned int m_options;
};

template<>
LogWriter& LogWriter::operator<<<Manipulators>(const Manipulators& manipulator);

template<typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& pair)
{
    return out << '(' << pair.first << ", " << pair.second << ')';
}

template<typename T>
inline std::ostream& printContainer(std::ostream& out, const T& container)
{
    out << '[';
    if (container.size()) {
        typename T::const_iterator it = container.begin();
        out << *it;
        ++it;
        for (; it != container.end(); ++it)
            out << ", " << *it;
    }
    out << ']';
    return out;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, const std::list<T>& list)
{
    return printContainer(out, list);
}

template<typename T>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T>& vector)
{
    return printContainer(out, vector);
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
        *this << Yellow << "WARNING" << NoColor << " :: ";
    }
};

class Error
{
public:
    Error(const std::string& description)
        : m_description(description)
    {
    }
    std::string description() const { return m_description; }
    void show() const;
private:
    std::string m_description;
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
        if (level > verbosityLevel)
            m_options |= Quiet;
    }
};

class Log
{
public:
    Log(const std::string& fileName);
    template<typename T>
    LogWriter operator<<(const T& t)
    {
        return LogWriter(m_stream, LogWriter::NoBreak) << t;
    }
private:
    std::ofstream m_stream;
};


#endif // LOGGER_H
