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

#ifndef COMPILABLETARGET_H
#define COMPILABLETARGET_H

#include "luatarget.h"
#include "joblistenner.h"

class LinkerOptions;
class CompilerOptions;

class CompilableTarget : public LuaTarget, public JobListenner
{
public:
    CompilableTarget(const std::string& targetName, MeiqueScript* script);
    ~CompilableTarget();
    void clean();
    void jobFinished(Job* job);
protected:
    JobQueue* doRun(Compiler* compiler);
private:
    CompilerOptions* m_compilerOptions;
    LinkerOptions* m_linkerOptions;
    std::map<Job*, StringList> m_job2Sources;

    void fillCompilerAndLinkerOptions();
    StringList getFileDependencies(const std::string& source);
    void preprocessFile(const std::string& source, const std::string& baseDir, StringList* deps);
};

#endif
