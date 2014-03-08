/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef COMPILERFACTORY_H
#define COMPILERFACTORY_H

#include <string>

class Compiler;
class CompilerFactory
{
public:
    /// Returns a null terminated array of available compiler names.
    static const char** availableCompilers();
    static Compiler* createCompiler(const std::string& compiler);
    static Compiler* findCompiler();
private:
    CompilerFactory(const CompilerFactory&) = delete;
};
#endif
