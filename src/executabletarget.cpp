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

#include "executabletarget.h"
#include "os.h"
#include "compiler.h"
#include "oscommandjob.h"
#include "logger.h"
#include "linkeroptions.h"
#include "meiquecache.h"

ExecutableTarget::ExecutableTarget(const std::string& targetName, MeiqueScript* script): CompilableTarget(targetName, script)
{
    setOutputFileName(cache()->compiler()->nameForExecutable(name()));
}

#if 0
JobQueue* ExecutableTarget::doRun(Compiler* compiler)
{
    StringList objects;
    JobQueue* queue = createCompilationJobs(compiler, &objects);
    if (!queue->isEmpty() || !OS::fileExists(outputFileName())) {
        std::string buildDir = OS::pwd();
        std::string exeName = compiler->nameForExecutable(name());
        OSCommandJob* job = new OSCommandJob(compiler->link(exeName, objects, linkerOptions()));
        job->setWorkingDirectory(buildDir);
        job->setName(exeName);
        job->setType(Job::Linking);
        job->setDependencies(queue->idleJobs());
        queue->addJob(job);
    }
    return queue;
}
#endif

void ExecutableTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    CompilableTarget::fillCompilerAndLinkerOptions(compilerOptions, linkerOptions);
    linkerOptions->setLinkType(LinkerOptions::Executable);
}
