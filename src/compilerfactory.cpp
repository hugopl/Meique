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
#include "gcc.h"
#include "logger.h"

static CompilerFactory factories[] = {
    Gcc::factory()
};

Compiler* createCompiler(const std::string& compilerId)
{
    for (CompilerFactory& factory : factories) {
        if (compilerId == factory.id)
            return factory.create();
    }
    throw Error("Unable to create compiler wrapper for " + compilerId);
    return 0;
}

const char* findCompilerId()
{
    std::string fullName;
    for (CompilerFactory& factory : factories) {
        if (factory.probe(fullName)) {
            Notice() << "-- Found " << fullName;
            return factory.id;
        }
    }
    throw Error("No usable compilers were found!");
    return 0;
}
