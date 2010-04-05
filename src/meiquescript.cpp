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
#include "target.h"
#include "compilabletarget.h"
#include "librarytarget.h"
#include "customtarget.h"

enum TargetTypes {
    COMPILABLE_TARGET = 1,
    LIBRARY_TARGET,
    CUSTOM_TARGET
};

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
"    if type(name) ~= \"table\" then\n"
"        o._files = {}\n"
"        _meiqueAllTargets[tostring(name)] = o\n"
"    end\n"
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
"    o._type = 3\n"
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
"    o._type = 1\n"
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
"    o._type = 2\n"
"    return o\n"
"end\n"
"\n"
"_meiqueOptions = {}\n"
"function AddOption(name, description, defaultValue)\n"
"    AbortIf(name == nil, \"An option can't have a nil name\")\n"
"    _meiqueOptions[name] = {description, defaultValue}\n"
"end\n";

MeiqueScript::MeiqueScript(const Config& config) : m_config(config)
{
    exportApi();
    m_scriptName = m_config.sourceRoot()+"/meique.lua";
    translateLuaError(luaL_loadfile(m_L, m_scriptName.c_str()));
}

MeiqueScript::~MeiqueScript()
{
    TargetsMap::const_iterator it = m_targets.begin();
    for (; it != m_targets.end(); ++it)
        delete it->second;
}

static int meiqueErrorHandler(lua_State* L)
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

    if (m_config.isInBuildMode())
        extractTargets();
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
MeiqueOption lua_tocpp<MeiqueOption>(lua_State* L, int index)
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

void MeiqueScript::extractTargets()
{
    lua_getglobal(m_L, "_meiqueAllTargets");
    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    while (lua_next(m_L, tableIndex) != 0) {
        // Get target type
        lua_getfield(m_L, -1, "_type");
        int targetType = lua_tocpp<int>(m_L, -1);
        lua_pop(m_L, 1);

        std::string targetName = lua_tocpp<std::string>(m_L, -2);

        Target* target;
        switch (targetType) {
            case COMPILABLE_TARGET:
                target = new CompilableTarget(targetName, this);
                break;
            case LIBRARY_TARGET:
                target = new LibraryTarget(targetName, this);
                break;
            case CUSTOM_TARGET:
                target = new CustomTarget(targetName, this);
                break;
            default:
                Error() << "Unknown target type for target " << targetName;
                break;
        };
        lua_pushlightuserdata(m_L, (void*) target);
        lua_insert(m_L, -2);
        lua_settable(m_L, LUA_REGISTRYINDEX);
        m_targets[targetName] = target;
    }
    // Create special "all" target.
    if (m_targets["all"])
        Error() << "You can't define a target with the name \"all\"! It's a reserved target name.";
     m_targets["all"] = new Target("all", this);
}

Target* MeiqueScript::getTarget(const std::string& name)
{
    TargetsMap::const_iterator it = m_targets.find(name);
    if (it == m_targets.end())
        Error() << "Target \"" << name << "\" not found!";
    return it->second;
}

TargetList MeiqueScript::targets() const
{
    TargetList list;
    TargetsMap::const_iterator it = m_targets.begin();
    for (; it != m_targets.end(); ++it)
        list.push_back(it->second);
    return list;
}
