/*
    This file is part of the Meique project
    Copyright (C) 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "luastate.h"
#include "logger.h"
#include "lauxlib.h"
#include "mutexlocker.h"

LuaState::LuaState()
{
    m_L = luaL_newstate();
    if (!m_L)
        throw Error("Can't create lua state");

    lua_pushlightuserdata(m_L, (void*) m_L);
    lua_pushlightuserdata(m_L, (void*) &m_mutex);
    lua_settable(m_L, LUA_REGISTRYINDEX);
}

LuaState::~LuaState()
{
    // Be sure that nobody is waiting for the lua mutex.
    std::lock_guard<std::mutex> lock(m_mutex);
    lua_close(m_L);
}
