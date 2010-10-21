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

#include "target.h"
#include "joblistenner.h"

class LinkerOptions;
class CompilerOptions;

class CompilableTarget : public Target, public JobListenner
{
public:
    CompilableTarget(const std::string& targetName, MeiqueScript* script);
    ~CompilableTarget();
    void clean();
    void jobFinished(Job* job);
    const CompilerOptions* compilerOptions() const { return m_compilerOptions; }
    const LinkerOptions* linkerOptions() const { return m_linkerOptions; }
    void setOutputFileName(const std::string& fileName) { m_outputFileName = fileName; }
    std::string outputFileName() const { return m_outputFileName; }
protected:
    JobQueue* createCompilationJobs(Compiler* compiler, StringList* objects);
    virtual void fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions);
    virtual void useIn(CompilableTarget* other, CompilerOptions* otherCompilerOptions, LinkerOptions* otherLinkerOptions) = 0;
private:
    std::map<Job*, StringList> m_job2Sources;
    CompilerOptions* m_compilerOptions;
    LinkerOptions* m_linkerOptions;
    std::string m_outputFileName;

    StringList getFileDependencies(const std::string& source, const StringList& systemIncludeDirs);
    void preprocessFile(const std::string& source, StringList& userIncludeDirs, const StringList& systemIncludeDirs, bool isSystemHeader, StringList* deps);
};

#endif
