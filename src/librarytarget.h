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

#ifndef LIBRARYTARGET_H
#define LIBRARYTARGET_H

#include "compilabletarget.h"
#include "linkeroptions.h"

class LibraryTarget : public CompilableTarget
{
public:
    LibraryTarget(const std::string& targetName, MeiqueScript* script);
    void useIn(CompilerOptions* otherCompilerOptions, LinkerOptions* otherLinkerOptions);
protected:
//    JobQueue* doRun(Compiler* compiler);
    void fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions);
private:
    LinkerOptions::LinkType m_linkType;
};

#endif
