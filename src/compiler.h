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

#ifndef COMPILER_H
#define COMPILER_H

#include "basictypes.h"
#include "os.h"

class LinkerOptions;
class CompilerOptions;

enum Language {
    CLanguage,
    CPlusPlusLanguage,
    UnsupportedLanguage
};

/**
* Try to identify the language of a file by its extension using case insensitive string comparison.
*/
Language identifyLanguage(const std::string& fileName);

class Compiler
{
public:
    Compiler() {}
    virtual ~Compiler() {}
    virtual std::string compile(const std::string& fileName, const std::string& output, const CompilerOptions* options) = 0;
    virtual std::string link(const std::string& output, const StringList& objects, const LinkerOptions* options) const = 0;
    virtual std::string nameForExecutable(const std::string& name) const = 0;
    virtual std::string nameForStaticLibrary(const std::string& name) const = 0;
    virtual std::string nameForSharedLibrary(const std::string& name) const = 0;
    virtual std::string nameForObject(const std::string& name, const std::string& target) const;
    virtual bool shouldCompile(const std::string& source, const std::string& output) const = 0;
private:
    Compiler(const Compiler&) = delete;
};

struct CompilerFactory {
    const char* id;
    bool (*probe)(std::string&);
    Compiler* (*create)();
};

#endif
