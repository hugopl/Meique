#ifndef LUACPPUTIL_H
#define LUACPPUTIL_H

#include <string>
extern "C" {
#include <lua.h>
}

/// Convert a lua type at index \p index to the C++ type \p T.
template<typename T>
T lua_tocpp(lua_State* L, int index);

template<>
inline std::string lua_tocpp<std::string>(lua_State* L, int index)
{
    const char* res = lua_tostring(L, index);
    return res ? res : std::string();
}

template<>
inline int lua_tocpp<int>(lua_State* L, int index)
{
    return lua_tointeger(L, index);
}

template<typename Map>
static void readLuaTable(lua_State* L, int tableIndex, Map& map)
{
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {
        typename Map::key_type key = lua_tocpp<typename Map::key_type>(L, -2);
        typename Map::mapped_type value = lua_tocpp<typename Map::mapped_type>(L, lua_gettop(L));
        map[key] = value;
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }
}

#endif
