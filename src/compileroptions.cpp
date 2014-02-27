/*
    This file is part of the Meique project
    Copyright (C) 2010-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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
#include "os.h"

CompilerOptions::CompilerOptions() : m_compileForLibrary(false), m_debugInfoEnabled(false)
{
}

void CompilerOptions::addIncludePath(const std::string& includePath)
{
    if (includePath.empty())
        return;
    m_includePaths.push_back(OS::normalizeDirPath(includePath));
}

void CompilerOptions::addIncludePaths(const StringList& includePaths)
{
    StringList::const_iterator it = includePaths.begin();
    for (; it != includePaths.end(); ++it)
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

void CompilerOptions::normalize()
{
    // FIXME: Can't sort include paths or custom flags because this can cause compilation problems
    //        on buggy projects.
    m_includePaths.sort();
    m_includePaths.erase(std::unique(m_includePaths.begin(), m_includePaths.end()), m_includePaths.end());
    m_defines.sort();
    m_defines.erase(std::unique(m_defines.begin(), m_defines.end()), m_defines.end());
    m_customFlags.sort();
    m_customFlags.erase(std::unique(m_customFlags.begin(), m_customFlags.end()), m_customFlags.end());
}

void CompilerOptions::merge(const CompilerOptions& other)
{
    copy(m_includePaths, other.m_includePaths);
    copy(m_defines, other.m_defines);
    copy(m_customFlags, other.m_customFlags);
}
