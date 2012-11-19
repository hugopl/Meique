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


#ifndef CMDLINE_H
#define CMDLINE_H

#include "basictypes.h"

class CmdLine
{
public:
    CmdLine(int argc, const char** argv);
    std::string arg(const std::string& name, const std::string& defaultValue = std::string(), bool* found = 0) const;
    bool boolArg(const std::string& name) const;
    int intArg(const std::string& name, int defaultValue = 0) const;
    int numberOfFreeArgs() const;
    std::string freeArg(int idx) const;
private:
    StringMap m_args;
    StringVector m_freeArgs;

    CmdLine(const CmdLine&);
    CmdLine& operator=(const CmdLine&);
};

#endif // CMDLINE_H
