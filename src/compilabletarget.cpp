/*
    This file is part of the Meique project
    Copyright (C) 2010-2013 Hugo Parente Lima <hugo.pl@gmail.com>

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
#include "meiquecache.h"
#include "os.h"
#include "filehash.h"
#include "compileroptions.h"
#include "linkeroptions.h"
#include "stdstringsux.h"
#include "oscommandjob.h"
#include "meiquescript.h"
#include "meiqueregex.h"

CompilableTarget::CompilableTarget(const std::string& targetName, MeiqueScript* script)
    : Target(targetName, script), m_compilerOptions(0), m_linkerOptions(0)
{
    m_compilerOptions = new CompilerOptions;
    m_linkerOptions = new LinkerOptions;
}

CompilableTarget::~CompilableTarget()
{
    delete m_compilerOptions;
    delete m_linkerOptions;
}

#if 0
JobQueue* CompilableTarget::createCompilationJobs(Compiler* compiler, StringList* objects)
{
    StringList files = this->files();

    if (files.empty())
        throw Error("Compilable target '" + name() + "' has no files!");

    fillCompilerAndLinkerOptions(m_compilerOptions, m_linkerOptions);

    JobQueue* queue = new JobQueue;
    std::string sourceDir = script()->sourceDir() + directory();
    std::string buildDir = OS::pwd();

    Language lang = files.size() ? identifyLanguage(*files.begin()) : UnsupportedLanguage;
    m_linkerOptions->setLanguage(lang);

    for (const std::string& fileName : files) {
        if (fileName.empty())
            continue;

        if (lang != identifyLanguage(fileName))
            throw Error("You can't mix two programming languages in the same target!");

        // Create necessary directories if needed.
        if (fileName.find_first_of('/'))
            OS::mkdir(OS::dirName(fileName));

        std::string source = OS::normalizeFilePath(fileName.at(0) == '/' ? fileName : sourceDir + fileName);
        std::string output = OS::normalizeFilePath(fileName + "." + name() + ".o");

        bool compileIt = OS::timestampCompare(source, output) < 0;

        StringList dependents = getFileDependencies(source);
        if (!compileIt)
            compileIt = cache()->isHashGroupOutdated(source, dependents);

        if (compileIt) {
            OSCommandJob* job = new OSCommandJob(compiler->compile(source, output, m_compilerOptions));
            job->addJobListenner(this);
            job->setWorkingDirectory(buildDir);
            job->setName(fileName);
            queue->addJob(job);
            m_job2Sources[job] = std::make_pair(source, dependents);
        }
        objects->push_back(output);
    }

    return queue;
}
#endif

void CompilableTarget::jobFinished(Job* job)
{
    if (!job->result())
        cache()->updateHashGroup(m_job2Sources[job].first, m_job2Sources[job].second);
    m_job2Sources.erase(job);
}

void CompilableTarget::preprocessFile(const std::string& source,
                                      StringList& userIncludeDirs,
                                      StringList* deps)
{
    static Regex regex("^\\s*#\\s*include\\s*[<\"]([^\">]+)[\">]");

    if (source.empty())
        return;

    std::string absSource;
    std::ifstream fp;

    if (source[0] == '/') {
        absSource = source;
        fp.open(source.c_str());
    } else {
        for (const std::string& dir : userIncludeDirs) {
            absSource = dir + source;
            fp.open(absSource.c_str());
            if (fp) {
                absSource = OS::normalizeFilePath(absSource);
                break;
            }
        }
    }

    if (!fp || !fp.is_open())
        return;

    if (std::find(deps->begin(), deps->end(), absSource) != deps->end())
        return;

    deps->push_back(absSource);

    std::string line;

    while (fp) {
        std::getline(fp, line);

        // Avoid regex execution non-preprocessor lines
        if (line.find_first_of('#') == std::string::npos)
            continue;

        if (fp && regex.match(line)) {
            userIncludeDirs.pop_front();
            userIncludeDirs.push_front(OS::dirName(absSource));
            preprocessFile(regex.group(1, line), userIncludeDirs, deps);
        }
    }
}

StringList CompilableTarget::getFileDependencies(const std::string& source)
{
    std::string baseDir = script()->sourceDir() + directory();
    StringList dependents;
    // FIXME: There is a large room for improviments here, we need to cache some results
    //        to avoid doing a lot of things twice.
    StringList includePaths = m_compilerOptions->includePaths();
    includePaths.push_front(baseDir);
    preprocessFile(source, includePaths, &dependents);

    if (!dependents.empty())
        dependents.pop_front();
    return dependents;
}

static void readMeiquePackageOnStack(lua_State* L, CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    StringMap map;
    readLuaTable(L, lua_gettop(L), map);

    compilerOptions->addIncludePaths(split(map["includePaths"]));
    compilerOptions->addCustomFlag(map["cflags"]);
    linkerOptions->addCustomFlag(map["linkerFlags"]);
    linkerOptions->addLibraryPaths(split(map["libraryPaths"]));
    linkerOptions->addLibraries(split(map["linkLibraries"]));
}

void CompilableTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    const std::string sourcePath = script()->sourceDir() + directory();

    // Add source dir in the include path
    compilerOptions->addIncludePath(sourcePath);

    // Add info from global package
    lua_State* L = luaState();
    lua_getglobal(L, "_meiqueGlobalPackage");
    readMeiquePackageOnStack(L, compilerOptions, linkerOptions);

    // Get the package info
    getLuaField("_packages");
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        if (lua_istable(L, -1))
            readMeiquePackageOnStack(L, compilerOptions, linkerOptions);
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1);

    if (cache()->buildType() == MeiqueCache::Debug)
        compilerOptions->enableDebugInfo();
    else
        compilerOptions->addDefine("NDEBUG");

    StringList list;
    // explicit include directories
    getLuaField("_incDirs");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    StringList::iterator it = list.begin();
    for (; it != list.end(); ++it) {
        if (!it->empty() && it->at(0) != '/')
            *it = sourcePath + *it;
    }
    compilerOptions->addIncludePaths(list);

    // explicit link libraries
    getLuaField("_linkLibraries");
    list.clear();
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    linkerOptions->addLibraries(list);

    // explicit library include dirs
    getLuaField("_libDirs");
    list.clear();
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    linkerOptions->addLibraryPaths(list);

    // other targets
    list.clear();
    getLuaField("_targets");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    for (const std::string& targetName : list) {
        CompilableTarget* target = dynamic_cast<CompilableTarget*>(script()->getTarget(targetName));
        if (target && target != this)
            target->fillCompilerAndLinkerOptions(compilerOptions, linkerOptions);
    }

    // Add build dir in the include path
    m_compilerOptions->addIncludePath(script()->buildDir() + directory());
    m_compilerOptions->normalize();
}

StringList CompilableTarget::includeDirectories()
{
    lua_State* L = luaState();

    StringList list;
    // explicit include directories
    getLuaField("_incDirs");
    readLuaList(L, lua_gettop(L), list);
    lua_pop(L, 1);
    list.sort();

    getLuaField("_packages");
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        if (lua_istable(L, -1)) {
            StringMap map;
            readLuaTable(L, lua_gettop(L), map);
            StringList incDirs(split(map["includePaths"]));
            list.merge(incDirs);
        }
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1);

    return list;
}

void CompilableTarget::clean()
{
    // get sources
    getLuaField("_files");
    StringList files;
    lua_State* L = luaState();
    readLuaList(L, lua_gettop(L), files);
    lua_pop(L, 1);

    for (std::string fileName : files) {
        fileName += "." + name() + ".o";
        if (fileName[0] != '/')
            fileName.insert(0, directory());
        OS::rm(fileName);
    }
}

void CompilableTarget::doTargetInstall(const std::string& destDir)
{
    // FIXME: We need to write special version for this on Windows when using shared libraries...
    //        or outputFileName() can return a list instead of a single entry.
    std::string targetFile = OS::normalizeFilePath(directory() + outputFileName());
    OS::install(targetFile, destDir);
}

void CompilableTarget::doTargetUninstall(const std::string& destDir)
{
    // FIXME: We need to write special version for this on Windows when using shared libraries...
    //        or outputFileName() can return a list instead of a single entry.
    std::string targetFile = OS::normalizeFilePath(directory() + outputFileName());
    OS::uninstall(destDir + outputFileName());
}
