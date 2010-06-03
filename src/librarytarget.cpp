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

#include "librarytarget.h"
#include "os.h"
#include "jobqueue.h"
#include "compiler.h"
#include "job.h"
#include "linkeroptions.h"
#include "luacpputil.h"
#include "logger.h"
#include "compileroptions.h"

LibraryTarget::LibraryTarget(const std::string& targetName, MeiqueScript* script): CompilableTarget(targetName, script)
{
}

JobQueue* LibraryTarget::doRun(Compiler* compiler)
{
    StringList objects;
    JobQueue* queue = createCompilationJobs(compiler, &objects);
    std::string libName;
    if (linkerOptions()->linkType() == LinkerOptions::SharedLibrary)
        libName = compiler->nameForSharedLibrary(name());
    else
        libName = compiler->nameForStaticLibrary(name());

    if (!queue->isEmpty() || !OS::fileExists(libName)) {

        std::string buildDir = OS::pwd();
        Job* job = compiler->link(libName, objects, linkerOptions());
        job->setWorkingDirectory(buildDir);
        job->setDescription("Linking library " + libName);
        job->setDependencies(queue->idleJobs());
        queue->addJob(job);
    }
    return queue;
}

void LibraryTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    CompilableTarget::fillCompilerAndLinkerOptions(compilerOptions, linkerOptions);
    compilerOptions->setCompileForLibrary(true);
    getLuaField("_libType");
    int libType = lua_tocpp<int>(luaState(), -1);
    lua_pop(luaState(), 1);
    switch (libType) {
        case 1:
            linkerOptions->setLinkType(LinkerOptions::SharedLibrary);
            break;
        case 2:
            linkerOptions->setLinkType(LinkerOptions::StaticLibrary);
            break;
        default:
            Error() << "Unknown library type! " << libType;
    }
}
