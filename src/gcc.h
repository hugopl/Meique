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

#ifndef GCC_H
#define GCC_H
#include "compiler.h"

class Gcc : public Compiler
{
public:
    Gcc();
    const char* name() const { return "Gcc"; }
    std::string fullName() const { return m_fullName; }
    bool isAvailable() const { return m_isAvailable; }
    OSCommandJob* compile(const std::string& fileName, const std::string& output, const CompilerOptions* options) const;
    OSCommandJob* link(const std::string& output, const StringList& objects, const LinkerOptions* options) const;
    std::string nameForExecutable(const std::string& name) const;
    std::string nameForStaticLibrary(const std::string& name) const;
    std::string nameForSharedLibrary(const std::string& name) const;
    StringList defaultIncludeDirs() const { return m_defaultIncludeDirs; }
private:
    bool m_isAvailable;
    std::string m_fullName;
    std::string m_version;
    StringList m_defaultIncludeDirs;
};

#endif
