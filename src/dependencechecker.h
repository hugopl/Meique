/*
    This file is part of the Meique project
    Copyright (C) 2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef DEPENDENCECHECKER_H
#define DEPENDENCECHECKER_H

#include "basictypes.h"

class DependenceChecker
{
public:
    void setWorkingDirectory(const std::string& cwd) { m_cwd = cwd; }
    bool shouldCompile(const std::string& file, const std::string& output);
private:
    std::string m_cwd;

    StringList getDependencies(const std::string& file);
    void preprocessFile(const std::string& source, StringList& includeDirs, StringList& deps);
};

#endif
