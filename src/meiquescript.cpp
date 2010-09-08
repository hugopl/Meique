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

#include <string>
#include <cstring>
#include <cassert>
#include <algorithm>

#include "config.h"
#include "logger.h"
#include "luacpputil.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"
#include "target.h"
#include "compilabletarget.h"
#include "librarytarget.h"
#include "customtarget.h"
#include "os.h"
#include "stdstringsux.h"
#include "maintarget.h"
#include "executabletarget.h"

enum TargetTypes {
    EXECUTABLE_TARGET = 1,
    LIBRARY_TARGET,
    CUSTOM_TARGET
};

const char meiqueApi[] = "\n"
"function abortIf(var, message)\n"
"    if var then\n"
"       error(message, 0)\n"
"    end\n"
"end\n"
"\n"
"function instanceOf(instance, desiredType)\n"
"    if type(instance) ~= 'table' then\n"
"        return false\n"
"    end\n"
"    mt = getmetatable(instance)\n"
"    while mt ~= nil do\n"
"        if mt == desiredType then\n"
"            return true\n"
"        end\n"
"        mt = getmetatable(mt)\n"
"    end\n"
"    return false\n"
"end\n"
"\n"
"-- Simple target\n"
"Target = { }\n"
"_meiqueAllTargets = {}\n"
"_meiqueCurrentDir = {'.'}\n"
"\n"
"-- Object used for disabled scopes\n"
"_meiqueNone = {}\n"
"setmetatable(_meiqueNone, _meiqueNone)\n"
"_meiqueNone.__call = function(func, ...)\n"
"    return _meiqueNone\n"
"end\n"
"_meiqueNone.__index = _meiqueNone.__call\n"
"_meiqueNone.__eval = function(table)\n"
"    return false\n"
"end\n"
"\n"
"-- Object used for enabled scopes\n"
"_meiqueNotNone = {}\n"
"setmetatable(_meiqueNotNone, _meiqueNotNone)\n"
"_meiqueNotNone.__index = function(table, key)\n"
"    return function() return _G[key] end\n"
"end\n"
"setmetatable(_meiqueNotNone, _meiqueNotNone)\n"
"\n"
"-- Scopes: build types\n"
"DEBUG = _meiqueNone\n"
"RELEASE = _meiqueNone\n"
"\n"
"-- Scopes: Platforms\n"
"LINUX = _meiqueNone\n"
"UNIX = _meiqueNone\n"
"WINDOWS = _meiqueNone\n"
"MACOSX = _meiqueNone\n"
"\n"
"-- Scopes: Compilers\n"
"GCC = _meiqueNone\n"
"MSVC = _meiqueNone\n"
"MINGW = _meiqueNone\n"
"\n"
"function addSubDirectory(dir)\n"
"    local strDir = tostring(dir)\n"
"    table.insert(_meiqueCurrentDir, dir)\n"
"    local fileName = table.concat(_meiqueCurrentDir, '/')..'/meique.lua'\n"
"    local func, error = loadfile(fileName, fileName)\n"
"    abortIf(func == nil, error)\n"
"    func()\n"
"    table.remove(_meiqueCurrentDir)\n"
"end\n"
"\n"
"function Target:new(name)\n"
"    abortIf(name == nil and name ~= Target, \"Target name can't be nil\")\n"
"    o = {}\n"
"    setmetatable(o, Target)\n"
"    Target.__index = Target\n"
"    if type(name) ~= \"table\" then\n"
"        abortIf(name == 'all' or name == 'clean' or name == 'install', '\"all\", \"clean\" and \"install\" are reserved target names!')\n"
"        o._name = name\n"
"        o._files = {}\n"
"        o._deps = {}\n"
"        o._dir = table.concat(_meiqueCurrentDir, '/')\n"
"        _meiqueAllTargets[tostring(name)] = o\n"
"    end\n"
"    return o\n"
"end\n"
"\n"
"function Target:addFiles(...)\n"
"    for i,file in ipairs(arg) do\n"
"        string.gsub(file, '([^%s]+)', function(f) table.insert(self._files, f) end)\n"
"    end\n"
"end\n"
"\n"
"function Target:addDependency(target)\n"
"    abortIf(not instanceOf(target, Target), 'Expected a target, got something different.')\n"
"    table.insert(self._deps, target._name)\n"
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
"    o._packages = {}\n"
"    o._targets = {}\n"
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
"        string.gsub(file, '([^%s]+)', function(f) table.insert(self._linkLibraries, f) end)\n"
"    end\n"
"end\n"
"\n"
"function CompilableTarget:usePackage(package)\n"
"    table.insert(self._packages, package)\n"
"end\n"
"\n"
"function CompilableTarget:useTarget(target)\n"
"    abortIf(not instanceOf(target, Library), 'You can only use library targets on other targets.')\n"
"    table.insert(self._targets, target._name)\n"
"    self:addDependency(target)\n"
"end\n"
"\n"
"-- Executable\n"
"Executable = CompilableTarget:new(Target)\n"
"function Executable:new(name)\n"
"    o = CompilableTarget:new(name)\n"
"    setmetatable(o, self)\n"
"    self.__index = self\n"
"    o._type = 1\n"
"    return o\n"
"end\n"
"\n"
"SHARED = 1\n"
"STATIC = 2\n"
"-- Library\n"
"Library = CompilableTarget:new(Target)\n"
"function Library:new(name, libType)\n"
"    o = CompilableTarget:new(name)\n"
"    setmetatable(o, self)\n"
"    self.__index = self\n"
"    o._libType = libType or SHARED\n"
"    o._type = 2\n"
"    return o\n"
"end\n"
"\n"
"_meiqueOptions = {}\n"
"function addOption(name, description, defaultValue)\n"
"    abortIf(name == nil, \"An option can't have a nil name\")\n"
"    _meiqueOptions[name] = {description, defaultValue}\n"
"end\n";

MeiqueScript::MeiqueScript(Config& config) : m_config(config)
{
    exportApi();
    m_scriptName = m_config.sourceRoot()+"meique.lua";
    translateLuaError(m_L, luaL_loadfile(m_L, m_scriptName.c_str()), m_scriptName);
}

MeiqueScript::~MeiqueScript()
{
    TargetsMap::const_iterator it = m_targets.begin();
    for (; it != m_targets.end(); ++it)
        delete it->second;
}

void MeiqueScript::exec()
{
    OS::ChangeWorkingDirectory dirChanger(m_config.sourceRoot());
    luaPCall(m_L, m_scriptName);

    if (m_config.isInBuildMode())
        extractTargets();
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
    translateLuaError(m_L, sanityCheck, m_scriptName);
    assert(!sanityCheck);

    // Enable debug/release scope
    enableScope(m_config.buildType() == Config::Debug ? "DEBUG" : "RELEASE");
    // Enable compiler scope
    std::string compiler = m_config.compiler()->name();
    std::transform(compiler.begin(), compiler.end(), compiler.begin(), ::toupper);
    enableScope(compiler.c_str());
    // Enable OS scopes
    StringList osVars = OS::getOSType();
    for (StringList::iterator it = osVars.begin(); it != osVars.end(); ++it)
        enableScope(it->c_str());

    lua_register(m_L, "findPackage", &findPackage);
    lua_settop(m_L, 0);
    // export user options as variables
    const StringMap& userOptions = config().userOptions();
    StringMap::const_iterator it = userOptions.begin();
    for (; it != userOptions.end(); ++it) {
        std::string varName = it->first;
        trim(varName);
        std::transform(varName.begin(), varName.end(), varName.begin(), ::toupper);
        stringReplace(varName, "-", "_");
        lua_pushstring(m_L, it->second.c_str());
        lua_setglobal(m_L, varName.c_str());
    }
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

        Target* target = 0;
        switch (targetType) {
            case EXECUTABLE_TARGET:
                target = new ExecutableTarget(targetName, this);
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
    Target* mainTarget = new MainTarget(this);
    m_targets[mainTarget->name()] = mainTarget;
}

Target* MeiqueScript::getTarget(const std::string& name) const
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

struct StrFilter
{
    StrFilter(const std::string& garbage) : m_garbage(garbage) {}
    void filter(std::string& str)
    {
        std::istringstream s(str);
        std::string token;
        std::string result;
        while (s) {
            s >> token;
            if (!s)
                break;
            if (token.find(m_garbage) == 0) {
                token.erase(0, 2);
                result += ' ';
                result += token;
            } else {
                Debug() << "Discarding \"" << token << "\" from pkg-config link libraries.";
            }
        }
        trim(result);
        str = result;
    }
    std::string m_garbage;
};

int MeiqueScript::findPackage(lua_State* L)
{
    const char PKGCONFIG[] = "pkg-config";
    int nargs = lua_gettop(L);
    if (nargs < 1 || nargs > 2)
        LuaError(L) << "findPackage(name [, version]) called with wrong number of arguments.";
    std::string pkgName = lua_tocpp<std::string>(L, 1);
    std::string version = lua_tocpp<std::string>(L, 2);
    StringList args;

    // Check if the package exists
    args.push_back(pkgName);
    // TODO: Interpret >, >=, < and <= from the version expression
    if (!version.empty())
        args.push_back("--atleast-version="+version);
    int retval = OS::exec(PKGCONFIG, args);
    if (retval)
        LuaError(L) << pkgName << " package not found!";

    // Get config options
    const char* pkgConfigCmds[] = {"--libs-only-L",
                                   "--libs-only-l",
                                   "--libs-only-other",
                                   "--cflags-only-I",
                                   "--cflags-only-other",
                                   "--modversion"
                                  };
    const char* names[] = {"libraryPaths",
                           "linkLibraries",
                           "linkerFlags",
                           "includePaths",
                           "cflags",
                           "version"
                          };

    StrFilter libFilter("-l");
    StrFilter includeFilter("-I");
    StrFilter* filters[] = {
                            0,
                            &libFilter,
                            0,
                            &includeFilter,
                            0,
                            0,
                          };
    const int N = sizeof(pkgConfigCmds)/sizeof(const char*);
    lua_settop(L, 0);
    lua_createtable(L, 0, N);
    assert(sizeof(pkgConfigCmds) == sizeof(names));
    for (int i = 0; i < N; ++i) {
        std::string output;
        args.push_back(pkgConfigCmds[i]);
        OS::exec("pkg-config", args, &output);
        args.pop_back();
        trim(output);
        if (filters[i])
            filters[i]->filter(output);
        lua_pushstring(L, output.c_str());
        lua_setfield(L, -2, names[i]);
    }

    return 1;
}

void MeiqueScript::enableScope(const char* scopeName)
{
    lua_State* L = luaState();
    lua_getglobal(L, "_meiqueNotNone");
    lua_setglobal(L, scopeName);
}

