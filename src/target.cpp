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
#include "luacpputil.h"
extern "C" {
    #include <lua.h>
}
#include "logger.h"
#include "meiquescript.h"
#include "os.h"

Target::Target(const std::string& name, MeiqueScript* script)
              : m_name(name), m_script(script), m_dependenciesCached(false)
{
}

Target::~Target()
{
}

TargetList Target::dependencies()
{
    if (!m_dependenciesCached) {
        Notice() << "Scanning dependencies of target \"" << m_name << '"';
        if (m_name == "all") {
            TargetList targets = m_script->targets();
            targets.remove(this);
            return targets;
        } else {
        }
    }
    return m_dependencies;
}

void Target::run(Compiler* compiler)
{
    Notice() << "Running target \"" << m_name << '"';
    // TODO: Support parallel jobs
    TargetList deps = dependencies();
    TargetList::iterator it = deps.begin();
    for (; it != deps.end(); ++it)
        (*it)->run(compiler);

    if (m_name != "all") {
        OS::mkdir(directory());
        OS::ChangeWorkingDirectory dirChanger(directory());
        doRun(compiler);
    }
}

void Target::doRun(Compiler*)
{
}

lua_State* Target::luaState()
{
    return m_script->luaState();
}

Config& Target::config()
{
    return m_script->config();
}

const std::string& Target::directory()
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

