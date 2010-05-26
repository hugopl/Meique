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

#ifndef TARGET_H
#define TARGET_H
#include "basictypes.h"

class JobQueue;
class Compiler;

class Target
{
public:
    Target(const std::string& name);
    virtual ~Target();
    /// Returns a list with all target dependencies
    virtual TargetList dependencies() = 0;
    /// Get the target job queue
    JobQueue* run(Compiler* compiler);
    /// Returns the target's name
    const std::string& name() const { return m_name; }
    /// Returns the target directory (relative path to source root)
    virtual const std::string directory() = 0;
    /// Clean this target
    virtual void clean() {}
protected:
    virtual JobQueue* doRun(Compiler* compiler);
private:
    std::string m_name;

    Target(const Target&);
    Target& operator=(const Target&);
};

#endif
