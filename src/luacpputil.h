/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef LUACPPUTIL_H
#define LUACPPUTIL_H

#include <string>
#include <sstream>
#include <list>
#include <cassert>
#include "lua.h"
#include "basictypes.h"

#define LuaLeakCheck(L) LuaLeakCheckImpl _luaLeakChecker(__PRETTY_FUNCTION__, (L))
class LuaLeakCheckImpl
{
public:
    LuaLeakCheckImpl(const char* func, lua_State* L);
    ~LuaLeakCheckImpl();
private:
    const char* m_func;
    lua_State* m_L;
    int m_stackSize;
};

template<typename List>
void readLuaList(lua_State* L, int tableIndex, List& list);

/// Convert a lua type at index \p index to the C++ type \p T.
template<typename T>
T lua_tocpp(lua_State* L, int index);

template<>
inline std::string lua_tocpp<std::string>(lua_State* L, int index)
{
    if (lua_type(L, index) == LUA_TBOOLEAN)
        return lua_toboolean(L, index) ? "true" : "false";
    const char* res = lua_tostring(L, index);
    return res ? res : std::string();
}

template<>
inline int lua_tocpp<int>(lua_State* L, int index)
{
    return lua_tointeger(L, index);
}

template<>
inline bool lua_tocpp<bool>(lua_State* L, int index)
{
    return lua_toboolean(L, index);
}

template<>
inline StringList lua_tocpp<StringList>(lua_State* L, int index)
{
    StringList list;
    readLuaList(L, index, list);
    return list;
}

template<typename Map>
void readLuaTable(lua_State* L, int tableIndex, Map& map)
{
    assert(tableIndex >= 0);
    assert(lua_istable(L, tableIndex));

    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        typename Map::key_type key = lua_tocpp<typename Map::key_type>(L, -2);
        typename Map::mapped_type value = lua_tocpp<typename Map::mapped_type>(L, lua_gettop(L));
        map[key] = value;
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

template<typename List>
void readLuaTableKeys(lua_State* L, int tableIndex, List& list)
{
    assert(tableIndex >= 0);
    assert(lua_istable(L, tableIndex));

    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        typename List::value_type key = lua_tocpp<typename List::value_type>(L, -2);
        list.push_back(key);
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

void createLuaTable(lua_State* L, const StringMap& map);
void createLuaArray(lua_State* L, const StringList& list);

template<typename List>
void readLuaList(lua_State* L, int tableIndex, List& list)
{
    assert(tableIndex >= 0);
    assert(lua_istable(L, tableIndex));

    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        typename List::value_type value = lua_tocpp<typename List::value_type>(L, lua_gettop(L));
        list.push_back(value);
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

template<typename T>
T getField(lua_State* L, const char* key, int tableIndex = -1)
{
    lua_getfield(L, tableIndex, key);
    T retval = lua_tocpp<T>(L, -1);
    lua_pop(L, 1);
    return retval;
}

void luaError(lua_State* L, const std::string& msg);

/// Translate a lua error code into an C++ exception.
void translateLuaError(lua_State* L, int code, const std::string& scriptName);
void luaPCall(lua_State* L, int nargs = 0, int nresults = 0, const std::string& scriptName = std::string());

#endif
