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

#include "luacpputil.h"
#include <cstring>
#include "logger.h"
#include "lauxlib.h"

static int meiqueErrorHandler(lua_State* L)
{
    int level = 2;
    lua_Debug ar;
    while (lua_getstack(L, level++, &ar)) {
        lua_getinfo(L, "Snl", &ar);
        if (std::strncmp("[string \"", ar.short_src, 9)) {
            lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
            lua_insert(L, -2); // swap values on stack
            lua_concat(L, lua_gettop(L));
            break;
        }
    }
    return 1;
}

void translateLuaError(lua_State* L, int code, const std::string& scriptName)
{
    switch (code) {
        case 0:
            return;
        case LUA_ERRRUN:
        case LUA_ERRSYNTAX:
            Error() << lua_tostring(L, -1);
        case LUA_ERRFILE:
            Error() << '"' << scriptName << "\" not found";
        case LUA_ERRMEM:
            Error() << "Lua memory allocation error.";
        case LUA_ERRERR:
            Error() << "Bizarre error: " << lua_tostring(L, -1);
    };
}

void luaPCall(lua_State* L, int nargs, int nresults, const std::string& scriptName)
{
    lua_pushcfunction(L, meiqueErrorHandler);
    int errorIndex = lua_gettop(L) - nargs - 1;
    lua_insert(L, errorIndex);
    int errorCode = lua_pcall(L, nargs, nresults, errorIndex);
    translateLuaError(L, errorCode, scriptName);
}

void createLuaTable(lua_State* L, const StringMap& map)
{
    lua_createtable(L, 0, map.size());
    StringMap::const_iterator it = map.begin();
    for (; it != map.end(); ++it) {
        lua_pushstring(L, it->second.c_str());
        lua_setfield(L, -2, it->first.c_str());
    }
}

void createLuaArray(lua_State* L, const StringList& list)
{
    lua_createtable(L, list.size(), 0);
    StringList::const_iterator it = list.begin();
    for (int i = 0; it != list.end(); ++it, ++i) {
        lua_pushstring(L, it->c_str());
        lua_rawseti(L, -2, i);
    }
}

