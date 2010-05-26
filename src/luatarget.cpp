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

#include "luatarget.h"
#include "luacpputil.h"
extern "C" {
    #include <lua.h>
}
#include "logger.h"
#include "meiquescript.h"

LuaTarget::LuaTarget(const std::string& name, MeiqueScript* script) : Target(name), m_script(script), m_dependenciesCached(false)
{
}

TargetList LuaTarget::dependencies()
{
    // TODO Calculate target dependencies!
    if (!m_dependenciesCached) {
    }
    return m_dependencies;
}

JobQueue* LuaTarget::doRun(Compiler* compiler)
{
    return Target::doRun(compiler);
}

lua_State* LuaTarget::luaState()
{
    return m_script->luaState();
}

Config& LuaTarget::config()
{
    return m_script->config();
}

const std::string LuaTarget::directory()
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

void LuaTarget::getLuaField(const char* field)
{
    lua_pushlightuserdata(luaState(), (void *)this);
    lua_gettable(luaState(), LUA_REGISTRYINDEX);
    lua_getfield(luaState(), -1, field);
    // remove table from stack
    lua_insert(luaState(), -2);
    lua_pop(luaState(), 1);
}

