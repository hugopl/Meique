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
#include "oscommandjob.h"
#include "linkeroptions.h"
#include "luacpputil.h"
#include "logger.h"
#include "compileroptions.h"
#include "config.h"
#include "meiquescript.h"

LibraryTarget::LibraryTarget(const std::string& targetName, MeiqueScript* script): CompilableTarget(targetName, script)
{
}

JobQueue* LibraryTarget::doRun(Compiler* compiler)
{
    // get link type
    getLuaField("_libType");
    int libType = lua_tocpp<int>(luaState(), -1);
    lua_pop(luaState(), 1);
    switch (libType) {
        case 1:
            m_linkType = LinkerOptions::SharedLibrary;
            setOutputFileName(compiler->nameForSharedLibrary(name()));
            break;
        case 2:
            m_linkType = LinkerOptions::StaticLibrary;
            setOutputFileName(compiler->nameForStaticLibrary(name()));
            break;
        default:
            Error() << "Unknown library type! " << libType;
    }
    lua_pop(luaState(), 1);

    StringList objects;
    JobQueue* queue = createCompilationJobs(compiler, &objects);

    if (!queue->isEmpty() || !OS::fileExists(outputFileName())) {
        std::string buildDir = OS::pwd();
        OSCommandJob* job = compiler->link(outputFileName(), objects, linkerOptions());
        job->setWorkingDirectory(buildDir);
        job->setDescription("Linking library " + outputFileName());
        job->setDependencies(queue->idleJobs());
        queue->addJob(job);
    }
    return queue;
}

void LibraryTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    CompilableTarget::fillCompilerAndLinkerOptions(compilerOptions, linkerOptions);
    linkerOptions->setLinkType(m_linkType);
    compilerOptions->setCompileForLibrary(true);
}

void LibraryTarget::useIn(CompilableTarget* other, CompilerOptions* otherCompilerOptions, LinkerOptions* otherLinkerOptions)
{
    std::string buildPath = script()->buildDir() + directory();
    std::string sourcePath = script()->sourceDir() + directory();
    otherCompilerOptions->addIncludePath(sourcePath);
    if (m_linkType == LinkerOptions::SharedLibrary) {
        otherLinkerOptions->addLibraryPath(buildPath);
        otherLinkerOptions->addLibrary(name());
    } else if (m_linkType == LinkerOptions::StaticLibrary) {
        otherLinkerOptions->addStaticLibrary(buildPath + outputFileName());
    }
}
