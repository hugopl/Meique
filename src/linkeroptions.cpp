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

#include "linkeroptions.h"

void LinkerOptions::addCustomFlag(const std::string& customFlag)
{
    m_customFlags.push_back(customFlag);
}

void LinkerOptions::addLibrary(const std::string& library)
{
    m_libraries.push_back(library);
}

void LinkerOptions::addLibraryPath(const std::string& libraryPath)
{
    m_libraryPaths.push_back(libraryPath);
}
