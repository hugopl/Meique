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

#include "target.h"
#include "logger.h"
#include "os.h"
#include "luacpputil.h"
#include "lua.h"
#include "meiquescript.h"
#include <stdlib.h>
#include "meiquecache.h"

Target::Target(const std::string& name, MeiqueScript* script) : m_name(name), m_ran(false), m_script(script), m_dependenciesCached(false)
{
    // Self register on Lua registry
    lua_pushlightuserdata(luaState(), (void*) this);
    lua_insert(luaState(), -2);
    lua_settable(luaState(), LUA_REGISTRYINDEX);
}

Target::~Target()
{
}
#if 0
JobQueue* Target::run(Compiler* compiler)
{
    lua_State* L = luaState();
    // execute pre-compile hook functions
    getLuaField("_preTargetCompileHooks");
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        // push the target to the stack, it'll be the arg.
        lua_pushlightuserdata(L, (void *)this);
        lua_gettable(L, LUA_REGISTRYINDEX);
        luaPCall(L, 1, 0, "[preTargetCompileHook]");
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(L, 1); // remove the table

    Notice() <<  "Getting jobs for target " << m_name << "... " << NoBreak;
    OS::mkdir(directory());
    OS::ChangeWorkingDirectory dirChanger(directory());
    JobQueue* queue = doRun(compiler);
    if (queue->isEmpty())
        Notice() << "it's up to date!";
    else
        Notice() << queue->jobCount() << " found!";
    m_ran = true;
    return queue;
}

JobQueue* Target::doRun(Compiler*)
{
    return new JobQueue;
}
#endif

TargetList Target::dependencies()
{
    if (!m_dependenciesCached) {
        getLuaField("_deps");
        StringList list;
        lua_State* L = luaState();
        readLuaList(L, lua_gettop(L), list);
        lua_pop(L, 1);
        StringList::iterator it = list.begin();
        for (; it != list.end(); ++it) {
            Target* target = script()->getTarget(*it);
            m_dependencies.push_back(target);
        }
        m_dependenciesCached = true;
    }
    return m_dependencies;
}

lua_State* Target::luaState()
{
    return m_script->luaState();
}

MeiqueCache* Target::cache()
{
    return m_script->cache();
}

const std::string Target::directory()
{
    if (m_directory.empty()) {
        getLuaField("_dir");
        m_directory = lua_tocpp<std::string>(luaState(), -1);
        lua_pop(luaState(), 1);
    }
    return m_directory;
}

void Target::getLuaField(const char* field)
{
    lua_State* L = luaState();
    lua_pushlightuserdata(L, (void *)this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_getfield(L, -1, field);
    // remove table from stack
    lua_insert(L, -2);
    lua_pop(L, 1);
}

void Target::setLuaField(const char* field)
{
    lua_State* L = luaState();
    lua_pushlightuserdata(L, (void *)this);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_insert(L, -2);
    lua_setfield(L, -2, field);
}

StringList Target::files()
{
    // get sources
    getLuaField("_files");
    StringList files;
    lua_State* L = luaState();
    readLuaList(L, lua_gettop(L), files);
    lua_pop(L, 1);
    return files;
}

void Target::addFiles(const StringList& files)
{
    lua_State* L = luaState();
    // get sources
    getLuaField("_files");
    int pos = lua_objlen(L, -1) + 1;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        lua_pushstring(L, it->c_str());
        lua_rawseti(L, -2, pos);  /* t[pos] = v */
        ++pos;
    }
    setLuaField("_files");
}

void Target::install()
{
    getLuaField("_installFiles");
    lua_State* L = luaState();
    std::list<StringList> installDirectives;
    readLuaList(L, lua_gettop(L), installDirectives);
    lua_pop(L, 1);

    if (installDirectives.empty())
        return;

    Notice() << Cyan << "Installing " << name() << "...";

    for (const StringList& directive : installDirectives) {
        const int directiveSize = directive.size();
        if (!directiveSize)
            continue;

        const std::string destDir = OS::normalizeDirPath(cache()->installPrefix() + directive.front());
        const std::string srcDir = cache()->sourceDir() + directory();

        if (directiveSize == 1) { // Target installation
            doTargetInstall(destDir);
        } else if (directiveSize > 1) { // custom file install
            StringList::const_iterator it2 = directive.begin();
            for (++it2; it2 != directive.end(); ++it2)
                OS::install(srcDir + *it2, destDir);
        }
    }
}

void Target::uninstall()
{
    getLuaField("_installFiles");
    lua_State* L = luaState();
    std::list<StringList> installDirectives;
    readLuaList(L, lua_gettop(L), installDirectives);
    lua_pop(L, 1);

    if (installDirectives.empty())
        return;

    Notice() << Cyan << "Uninstalling " << name() << "...";

    for (const StringList& directive : installDirectives) {
        const int directiveSize = directive.size();
        if (!directiveSize)
            continue;

        const std::string destDir = OS::normalizeDirPath(cache()->installPrefix() + directive.front());
        const std::string srcDir = cache()->sourceDir() + directory();

        if (directiveSize == 1) { // Target installation
            doTargetUninstall(destDir);
        } else if (directiveSize > 1) { // custom file install
            StringList::const_iterator it2 = directive.begin();
            for (++it2; it2 != directive.end(); ++it2)
                OS::uninstall(destDir + OS::baseName(*it2));
        }
    }
}
