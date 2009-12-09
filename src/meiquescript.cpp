#include "meiquescript.h"
#include "config.h"
#include "logger.h"
#include "luacpputil.h"
#include <string>
#include <cstring>
#include <cassert>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
#include <lua.h>
}

const char meiqueApi[] = "\n"
"function AbortIf(var, message, level)\n"
"    if var then\n"
"       error(message, 0)\n"
"    end\n"
"end\n"
"-- Simple target\n"
"Target = { }\n"
"_meiqueAllTargets = {}\n"
"\n"
"function Target:new(name)\n"
"    AbortIf(name == nil and name ~= Target, \"Target name can't be nil\")\n"
"    o = {}\n"
"    setmetatable(o, Target)\n"
"    Target.__index = Target\n"
"    o._files = {}\n"
"    o._name = name\n"
"    _meiqueAllTargets[name] = o\n"
"    return o\n"
"end\n"
"\n"
"function Target:addFiles(...)\n"
"    for i,file in ipairs(arg) do\n"
"        table.insert(self._files, file)\n"
"    end\n"
"end\n"
"\n"
"-- Custom target\n"
"CustomTarget = Target:new(Target)\n"
"\n"
"function CustomTarget:new(name, func)\n"
"    o = Target:new(name)\n"
"    setmetatable(o, self)\n"
"    self.__index = self\n"
"    o._func = func\n"
"    return o\n"
"end\n"
"\n"
"-- Compilable target\n"
"CompilableTarget = Target:new(Target)\n"
"\n"
"function CompilableTarget:new(name)\n"
"    o = Target:new(name)\n"
"    setmetatable(o, self)\n"
"    self.__index = self\n"
"    o._incDirs = {}\n"
"    o._libDirs = {}\n"
"    o._linkLibraries = {}\n"
"    return o\n"
"end\n"
"\n"
"function CompilableTarget:addIncludeDirs(...)\n"
"    for i,file in ipairs(arg) do\n"
"        table.insert(self._incDirs, file)\n"
"    end\n"
"end\n"
"\n"
"function CompilableTarget:addLibIncludeDirs(...)\n"
"    for i,file in ipairs(arg) do\n"
"        table.insert(self._libDirs, file)\n"
"    end\n"
"end\n"
"\n"
"function CompilableTarget:addLinkLibraries(...)\n"
"    for i,file in ipairs(arg) do\n"
"        table.insert(self._linkLibraries, file)\n"
"    end\n"
"end\n"
"\n"
"-- Executable\n"
"Executable = CompilableTarget:new(Target)\n"
"\n"
"SHARED = 1\n"
"STATIC = 2\n"
"-- Library\n"
"Library = CompilableTarget:new(Target)\n"
"function Library:new(name, flags)\n"
"    o = CompilableTarget:new(name)\n"
"    setmetatable(o, self)\n"
"    self.__index = self\n"
"    o._flags = flags\n"
"    return o\n"
"end\n"
"\n"
"_meiqueOptions = {}\n"
"function AddOption(name, description, defaultValue)\n"
"    AbortIf(name == nil, \"An option can't have a nil name\")\n"
"    _meiqueOptions[name] = {description, defaultValue}\n"
"end\n";

MeiqueScript::MeiqueScript(const Config& config)
{
    exportApi();
    m_scriptName = config.sourceRoot()+"/meique.lua";
    translateLuaError(luaL_loadfile(m_L, m_scriptName.c_str()));
}

int meiqueErrorHandler(lua_State* L)
{
    int level = 2;
    lua_Debug ar;
    while (lua_getstack(L, level++, &ar)) {
        lua_getinfo(L, "Snl", &ar);
        if (std::strcmp("[string \"...\"]", ar.short_src)) {
            lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
            lua_insert(L, -2); // swap values on stack
            lua_concat(L, lua_gettop(L));
            break;
        }
    }
    return 1;
}

void MeiqueScript::exec()
{
    int errorIndex = lua_gettop(m_L);
    lua_pushcfunction(m_L, meiqueErrorHandler);
    lua_insert(m_L, errorIndex);
    int errorCode = lua_pcall(m_L, 0, 0, 1);
    translateLuaError(errorCode);
}

void MeiqueScript::translateLuaError(int code)
{
    switch (code) {
    case 0:
        return;
    case LUA_ERRRUN:
        Error() << "Runtime error: " << lua_tostring(m_L, -1);
    case LUA_ERRFILE:
        Error() << '"' << m_scriptName << "\" not found";
    case LUA_ERRSYNTAX:
        Error() << "Syntax error: " << lua_tostring(m_L, -1);
    case LUA_ERRMEM:
        Error() << "Lua memory allocation error.";
    case LUA_ERRERR:
        Error() << "Bizarre error: " << lua_tostring(m_L, -1);
    };
}

void MeiqueScript::exportApi()
{
    // load some libs.
    luaopen_base(m_L);
    luaopen_string(m_L);
    luaopen_table(m_L);

    // export lua API
    int sanityCheck = luaL_loadstring(m_L, meiqueApi);
    assert(!sanityCheck);
    sanityCheck = lua_pcall(m_L, 0, 0, 0);
    translateLuaError(sanityCheck);
    assert(!sanityCheck);
    lua_settop(m_L, 0);
}

template<>
inline MeiqueOption lua_tocpp<MeiqueOption>(lua_State* L, int index)
{
    if (!lua_istable(L, index))
        Error() << "Expecting a lua table! Got " << lua_typename(L, lua_type(L, index));
    IntStrMap map;
    readLuaTable(L, index, map);
    return MeiqueOption(map[1], map[2]);
}

OptionsMap MeiqueScript::options()
{
    if (m_options.size())
        return m_options;

    lua_getglobal(m_L, "_meiqueOptions");
    int tableIndex = lua_gettop(m_L);
    if (!lua_istable(m_L, tableIndex))
        Error() << "Your script is evil! Do not declare variables starting with _meique!";

    readLuaTable(m_L, tableIndex, m_options);
    return m_options;
}
