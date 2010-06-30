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

#include "lualocker.h"
#include "lua.h"

LuaLocker::LuaLocker(lua_State* L)
{
    // get mutex.
    lua_pushlightuserdata(L, (void *)L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    m_mutex = reinterpret_cast<pthread_mutex_t*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    pthread_mutex_lock(m_mutex);
}

LuaLocker::~LuaLocker()
{
    pthread_mutex_unlock(m_mutex);
}
