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
#include "meiqueregex.h"

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

        // Create necessary directories if needed.
        if (it->find_first_of('/'))
            OS::mkdir(OS::dirName(*it));

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
    static Regex regex("^[ \t]*#[ \t]*include[ \t]*([<\"])([^\">]+)[\">]");

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

    std::string line;

    while (fp) {
        std::getline(fp, line);

        // Avoid regex execution non-preprocessor lines
        if (line.find_first_of('#') == std::string::npos)
            continue;

        if (fp && regex.match(line)) {
            bool isSystemInclude = regex.group(1, line) == "<";
            userIncludeDirs.pop_front();
            userIncludeDirs.push_front(OS::dirName(absSource));
            preprocessFile(regex.group(2, line), userIncludeDirs, systemIncludeDirs, isSystemInclude, deps);
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
    const std::string baseDir = config().sourceRoot() + directory();

    // Add source dir in the include path
    m_compilerOptions->addIncludePath(baseDir);

    // Get the package info
    getLuaField("_packages");
    lua_State* L = luaState();
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        if (lua_istable(L, -1)) {
            StringMap map;
            readLuaTable(L, lua_gettop(L), map);

            compilerOptions->addIncludePaths(split(map["includePaths"]));
            compilerOptions->addCustomFlag(map["cflags"]);
            linkerOptions->addCustomFlag(map["linkerFlags"]);
            linkerOptions->addLibraryPaths(split(map["libraryPaths"]));
            linkerOptions->addLibraries(split(map["linkLibraries"]));
        }
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
    lua_pop(L, 1);
    StringList::iterator it = list.begin();
    for (; it != list.end(); ++it) {
        if (!it->empty() && it->at(0) != '/')
            *it = baseDir + *it;
    }
    compilerOptions->addIncludePaths(list);

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
    it = list.begin();
    for (; it != list.end(); ++it) {
        CompilableTarget* target = dynamic_cast<CompilableTarget*>(script()->getTarget(*it));
        if (target)
            target->useIn(this, compilerOptions, linkerOptions);
    }

    // Add build dir in the include path
    m_compilerOptions->addIncludePath(config().buildRoot() + directory());
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
