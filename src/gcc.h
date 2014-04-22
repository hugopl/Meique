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
#include <unordered_map>

class Gcc : public Compiler
{
public:
    static CompilerFactory factory();

    std::string compile(const std::string& fileName, const std::string& output, const CompilerOptions* options);
    std::string link(const std::string& output, const StringList& objects, const LinkerOptions* options, const std::string& targetDirectory) const;
    std::string nameForExecutable(const std::string& name) const;
    std::string nameForStaticLibrary(const std::string& name) const;
    std::string nameForSharedLibrary(const std::string& name) const;
    bool shouldCompile(const std::string& source, const std::string& output) const;
private:
    typedef std::unordered_map<const CompilerOptions*, std::string> CompilerCommandCache;
    CompilerCommandCache m_compileCommandCache;
};

#endif
