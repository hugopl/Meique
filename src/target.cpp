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

#include "target.h"
#include "logger.h"
#include "os.h"
#include "jobqueue.h"
#include "luacpputil.h"
#include "lua.h"
#include "meiquescript.h"

Target::Target(const std::string& name, MeiqueScript* script) : m_name(name), m_ran(false), m_script(script)
{
}

Target::~Target()
{
}

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

    Notice() << magenta() << "Getting jobs for target " << m_name;
    OS::mkdir(directory());
    OS::ChangeWorkingDirectory dirChanger(directory());
    JobQueue* queue = doRun(compiler);
    if (queue->isEmpty())
        Notice() << "Nothing to do for " << m_name;
    m_ran = true;
    return queue;
}

JobQueue* Target::doRun(Compiler*)
{
    return new JobQueue;
}

TargetList Target::dependencies()
{
    if (!m_dependenciesCached) {
        getLuaField("_deps");
        StringList list;
        readLuaList(luaState(), lua_gettop(luaState()), list);
        lua_pop(luaState(), 1);
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

Config& Target::config()
{
    return m_script->config();
}

const std::string Target::directory()
{
    if (m_directory.empty()) {
        getLuaField("_dir");
        m_directory = lua_tocpp<std::string>(luaState(), -1);
        lua_pop(luaState(), 1);
        if (!m_directory.empty())
            m_directory += '/';
    }
    return m_directory;
}

void Target::getLuaField(const char* field)
{
    lua_pushlightuserdata(luaState(), (void *)this);
    lua_gettable(luaState(), LUA_REGISTRYINDEX);
    lua_getfield(luaState(), -1, field);
    // remove table from stack
    lua_insert(luaState(), -2);
    lua_pop(luaState(), 1);
}
