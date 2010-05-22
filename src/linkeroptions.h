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

#ifndef LINKEROPTIONS_H
#define LINKEROPTIONS_H
#include "basictypes.h"

class LinkerOptions
{
public:
    LinkerOptions() {}
    void addLibrary(const std::string& library);
    StringList libraries() const { return m_libraries; }
    void addLibraryPath(const std::string& libraryPath);
    StringList libraryPath() const { return m_libraryPaths; }
    void addCustomFlag(const std::string& customFlag);
    StringList customFlags() const { return m_customFlags; }
private:

    StringList m_libraries;
    StringList m_libraryPaths;
    StringList m_customFlags;

    LinkerOptions(const LinkerOptions&);
    LinkerOptions& operator=(const LinkerOptions&);
};

#endif // LINKEROPTIONS_H
