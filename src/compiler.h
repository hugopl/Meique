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

class LinkerOptions;
class CompilerOptions;
class Job;

/**
*   Interface for all compiler implementations.
*/
class Compiler
{
public:
    virtual ~Compiler() {}
    virtual const char* name() const = 0;
    virtual bool isAvailable() const = 0;
    virtual Job* compile(const std::string& fileName, const std::string& output, const CompilerOptions* options) const = 0;
    virtual Job* link(const std::string& output, const StringList& objects, const LinkerOptions* options) const = 0;
};

#endif
