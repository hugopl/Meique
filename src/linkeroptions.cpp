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

#include "linkeroptions.h"
#include <algorithm>

void LinkerOptions::addCustomFlag(const std::string& customFlag)
{
    m_customFlags.push_back(customFlag);
}

void LinkerOptions::addStaticLibrary(const std::string& library)
{
    m_staticLibraries.push_back(library);
}

void LinkerOptions::addLibrary(const std::string& library)
{
    m_libraries.push_back(library);
}

void LinkerOptions::addLibraries(const StringList& libraries)
{
    std::copy(libraries.begin(), libraries.end(), std::back_inserter(m_libraries));
}

void LinkerOptions::addLibraryPath(const std::string& libraryPath)
{
    m_libraryPaths.push_back(libraryPath);
}

void LinkerOptions::addLibraryPaths(const StringList& libraryPaths)
{
    std::copy(libraryPaths.begin(), libraryPaths.end(), std::back_inserter(m_libraryPaths));
}

void LinkerOptions::merge(const LinkerOptions& other)
{
    copy(m_libraries, other.m_libraries);
    copy(m_staticLibraries, other.m_staticLibraries);
    copy(m_libraryPaths, other.m_libraryPaths);
    copy(m_customFlags, other.m_customFlags);
}
