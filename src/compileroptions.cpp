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

#include "compileroptions.h"
#include <algorithm>

CompilerOptions::CompilerOptions() : m_compileForLibrary(false), m_debugInfoEnabled(false)
{
}

void CompilerOptions::addIncludePath(const std::string& includePath)
{
    if (includePath.empty())
        return;
    if (*includePath.rbegin() != '/')
        m_includePaths.push_back(includePath + '/');
    else
        m_includePaths.push_back(includePath);
}

void CompilerOptions::addIncludePaths(const StringList& includePaths)
{
    StringList::const_iterator it = includePaths.begin();
    for (; it != includePaths.begin(); ++it)
        addIncludePath(*it);
}

void CompilerOptions::addDefine(const std::string& define)
{
    m_defines.push_back(define);
}

void CompilerOptions::addCustomFlag(const std::string& customFlag)
{
    m_customFlags.push_back(customFlag);
}

