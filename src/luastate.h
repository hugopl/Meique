#ifndef LUASTATE_H
#define LUASTATE_H

struct lua_State;

/// RAII for lua state
class LuaState {
public:
    LuaState();
    ~LuaState();
    operator lua_State*() { return m_L; }
private:
    lua_State* m_L;
};

#endif
