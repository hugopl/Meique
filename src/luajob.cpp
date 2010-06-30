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

#include "luajob.h"
#include "lualocker.h"
#include "lua.h"
#include "luacpputil.h"
#include "logger.h"

LuaJob::LuaJob(lua_State* L, void* luaRegisterKey, const std::string& variable)
    : m_L(L), m_registerKey(luaRegisterKey), m_variable(variable)
{
}

int LuaJob::doRun()
{
    LuaLocker locker(m_L);

    // Get the lua function and put it on lua stack
    lua_pushlightuserdata(m_L, m_registerKey);
    lua_gettable(m_L, LUA_REGISTRYINDEX);
    lua_getfield(m_L, -1, m_variable.c_str());
    // remove table from stack
    lua_insert(m_L, -2);
    lua_pop(m_L, 1);

    try {
        luaPCall(m_L);
    } catch (const MeiqueError&) {
        MeiqueError::errorAlreadyset = false;
        return 1;
    }
    return 0;
}
