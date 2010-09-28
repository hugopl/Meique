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

#include "compilerfactory.h"
#include <cstring>
#include "gcc.h"
#include "logger.h"

const char** CompilerFactory::availableCompilers()
{
    static const char* compilers[] = {"Gcc", 0};
    return compilers;
}

Compiler* CompilerFactory::createCompiler(const std::string& compiler)
{
    if (compiler == "Gcc")
        return new Gcc;

    Error() << "Unable to create compiler wrapper for " << compiler;
    return 0;
}

Compiler* CompilerFactory::findCompiler()
{
    const char** compilers = availableCompilers();
    for (int i = 0; compilers[i]; ++i) {
        Compiler* compiler = createCompiler(compilers[i]);
        if (compiler->isAvailable()) {
            Notice() << "-- Found " << compiler->fullName();
            return compiler;
        }
        delete compiler;
    }
    Error() << "No usable compilers were found!";
    return 0;
}
