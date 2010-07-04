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

#include "compilabletarget.h"
#include <fstream>
#include <algorithm>

#include "luacpputil.h"
#include "compiler.h"
#include "logger.h"
#include "config.h"
#include "os.h"
#include "filehash.h"
#include "compileroptions.h"
#include "linkeroptions.h"
#include "stdstringsux.h"
#include "jobqueue.h"
#include "oscommandjob.h"
#include "meiquescript.h"

CompilableTarget::CompilableTarget(const std::string& targetName, MeiqueScript* script)
    : LuaTarget(targetName, script), m_compilerOptions(0), m_linkerOptions(0)
{
    m_compilerOptions = new CompilerOptions;
    m_linkerOptions = new LinkerOptions;
}

CompilableTarget::~CompilableTarget()
{
    delete m_compilerOptions;
    delete m_linkerOptions;
}

JobQueue* CompilableTarget::createCompilationJobs(Compiler* compiler, StringList* objects)
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);

    if (files.empty())
        Error() << "Compilable target '" << name() << "' has no files!";

    fillCompilerAndLinkerOptions(m_compilerOptions, m_linkerOptions);

    JobQueue* queue = new JobQueue;
    std::string sourceDir = config().sourceRoot() + directory();
    std::string buildDir = OS::pwd();

    StringList::const_iterator it = files.begin();
    Language lang = identifyLanguage(*it);
    m_linkerOptions->setLanguage(lang);
    for (; it != files.end(); ++it) {
        if (lang != identifyLanguage(*it))
            Error() << "You can't mix two programming languages in the same target!";

        std::string source = sourceDir + *it;
        std::string output = *it + ".o";

        bool compileIt = !OS::fileExists(output);
        StringList dependents = getFileDependencies(source, compiler->defaultIncludeDirs());
        if (!compileIt)
            compileIt = config().isHashGroupOutdated(dependents);

        if (compileIt) {
            OSCommandJob* job = compiler->compile(source, output, m_compilerOptions);
            job->addJobListenner(this);
            job->setWorkingDirectory(buildDir);
            job->setDescription("Compiling " + *it);
            queue->addJob(job);
            m_job2Sources[job] = dependents;
        }
        objects->push_back(output);
    }

    return queue;
}
void CompilableTarget::jobFinished(Job* job)
{
    if (!job->result())
        config().updateHashGroup(m_job2Sources[job]);
}

void CompilableTarget::preprocessFile(const std::string& source,
                                      StringList& userIncludeDirs,
                                      const StringList& systemIncludeDirs,
                                      bool isSystemHeader,
                                      StringList* deps)
{
    if (source.empty())
        return;

    std::string absSource;
    std::ifstream fp;

    if (source[0] == '/') {
        absSource = source;
        fp.open(source.c_str());
    } else {
        // It's a system header
        if (isSystemHeader) {
            StringList::const_iterator it = systemIncludeDirs.begin();
            for (; it != systemIncludeDirs.end(); ++it) {
                absSource = *it + source;
                fp.open(absSource.c_str());
                if (fp)
                    break;
            }
        }
        // It's a normal file or a system header not found in the system directories
        if (!fp && source[0] != '/') {
            StringList::const_iterator it = systemIncludeDirs.begin();
            for (; it != systemIncludeDirs.end(); ++it) {
                absSource = *it + source;
                fp.open(absSource.c_str());
                if (fp)
                    break;
            }
        }
    }

    if (fp) {
        if (std::find(deps->begin(), deps->end(), absSource) != deps->end())
            return;
        deps->push_back(absSource);
    } else {
        if (!isSystemHeader)
            Debug() << "Include file not found: " << source << " - " << userIncludeDirs.front();
        return;
    }

    std::string buffer1;
    std::string buffer2;

    while (!fp.eof()) {
        std::getline(fp, buffer1);
        trim(buffer1);

        if (buffer1[0] != '#')
            continue;
        buffer1.erase(0, 1);

        while (buffer1[buffer1.size() - 1] == '\\') {
            if (fp.eof())
                return;
            std::getline(fp, buffer2);
            buffer1.erase(buffer1.size() - 1, 1);
            buffer1 += buffer2;
            trim(buffer1);
        }

        if (buffer1.compare(0, 7, "include") != 0)
            continue;
        buffer1.erase(0, 7);
        trim(buffer1);
        bool isSystemInclude = buffer1[ 0 ] == '<' || buffer1[ 0 ] == '>';
        bool isLocalInclude = buffer1[ 0 ] == '"';
        if (isSystemInclude || isLocalInclude) {
            std::string includedFile = buffer1.substr(1, buffer1.find_first_of("\">") - 1);
            userIncludeDirs.pop_front();
            userIncludeDirs.push_front(OS::dirName(absSource));
            preprocessFile(includedFile, userIncludeDirs, systemIncludeDirs, isSystemInclude, deps);
        }
    }
}

StringList CompilableTarget::getFileDependencies(const std::string& source, const StringList& systemIncludeDirs)
{
    std::string baseDir = config().sourceRoot() + directory();
    StringList dependents;
    // FIXME: There is a large room for improviments here, we need to cache some results
    //        to avoid doing a lot of things twice.
    StringList includePaths = m_compilerOptions->includePaths();
    includePaths.push_front(baseDir);
    preprocessFile(source, includePaths, systemIncludeDirs, false, &dependents);
    return dependents;
}

void CompilableTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    // Get the package info
    getLuaField("_packages");
    lua_State* L = luaState();
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {

        StringMap map;
        readLuaTable(L, lua_gettop(L), map);

        compilerOptions->addIncludePath(map["includePaths"]);
        compilerOptions->addCustomFlag(map["cflags"]);
        linkerOptions->addCustomFlag(map["linkerFlags"]);
        linkerOptions->addLibraryPath(map["libraryPaths"]);
        linkerOptions->addLibraries(split(map["linkLibraries"]));
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1);

    if (config().buildType() == Config::Debug)
        compilerOptions->enableDebugInfo();
    else
        compilerOptions->addDefine("NDEBUG");

    StringList list;
    // explicit include directories
    getLuaField("_incDirs");
    list.clear();
    readLuaList(L, lua_gettop(L), list);
    compilerOptions->addIncludePaths(list);
    lua_pop(L, 1);

    // explicit link libraries
    getLuaField("_linkLibraries");
    list.clear();
    readLuaList(L, lua_gettop(L), list);
    linkerOptions->addLibraries(list);
    lua_pop(L, 1);

    // explicit library include dirs
    getLuaField("_libDirs");
    list.clear();
    readLuaList(L, lua_gettop(L), list);
    linkerOptions->addLibraryPaths(list);
    lua_pop(L, 1);

    // other targets
    list.clear();
    getLuaField("_targets");
    readLuaList(L, lua_gettop(L), list);
    StringList::iterator it = list.begin();
    for (; it != list.end(); ++it) {
        CompilableTarget* target = dynamic_cast<CompilableTarget*>(script()->getTarget(*it));
        if (target)
            target->useIn(this, compilerOptions, linkerOptions);
    }
}

void CompilableTarget::clean()
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it)
        OS::rm(directory() + *it + ".o");
}
