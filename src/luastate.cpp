#include "luastate.h"
#include "logger.h"
extern "C" {
#include <lauxlib.h>
}

LuaState::LuaState()
{
    m_L = luaL_newstate();
    if (!m_L)
        Error() << "Can't create lua state";
}

LuaState::~LuaState()
{
    lua_close(m_L);
}
