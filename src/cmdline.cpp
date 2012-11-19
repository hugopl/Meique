/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Hugo Parente Lima <hugo.pl@gmail.com>

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


#include "cmdline.h"
#include <cctype>
#include <sstream>

#include "logger.h"

CmdLine::CmdLine(int argc, const char** argv)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.find("--") == 0) {
            arg.erase(0, 2);
            size_t equalPos = arg.find("=");
            if (equalPos == std::string::npos)
                m_args[arg];
            else
                m_args[arg.substr(0, equalPos)] = arg.substr(equalPos + 1, arg.size() - equalPos);
        } else if (arg[0] == '-') { // short options
            arg.erase(0, 1);
            char previous = 0;
            char fakeStr[2] = {0, 0};
            int charsToConsume;
            while (!arg.empty()) {
                const char c = arg[0];
                if (std::isdigit(c) && previous) {
                    int value;
                    std::istringstream s(arg);
                    s >> value;
                    fakeStr[0] = previous;
                    charsToConsume = s.tellg();
                    m_args[fakeStr] = arg.substr(0, charsToConsume);
                    previous = 0;
                } else {
                    fakeStr[0] = c;
                    m_args[fakeStr] = std::string();
                    previous = c;
                    charsToConsume = 1;
                }
                arg.erase(0, charsToConsume);
            }
        } else {
            m_freeArgs.push_back(arg);
        }
    }
}

int CmdLine::numberOfFreeArgs() const
{
    return m_freeArgs.size();
}

std::string CmdLine::freeArg(int idx) const
{
    return m_freeArgs[idx];
}

std::string CmdLine::arg(const std::string& name, const std::string& defaultValue, bool* found) const
{
    StringMap::const_iterator it = m_args.find(name);
    if (it == m_args.end()) {
        if (found)
            *found = false;
        return defaultValue;
    }
    if (found)
        *found = true;
    return it->second;
}

int CmdLine::intArg(const std::string& name, int defaultValue) const
{
    StringMap::const_iterator it = m_args.find(name);
    if (it == m_args.end())
        return defaultValue;

    int value;
    std::istringstream s(it->second);
    s >> value;
    return value;
}

bool CmdLine::boolArg(const std::string& name) const
{
    StringMap::const_iterator it = m_args.find(name);
    return it != m_args.end();
}

