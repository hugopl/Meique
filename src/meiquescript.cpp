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
#include <fstream>

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
#include "executabletarget.h"
#include "meiqueregex.h"

enum TargetTypes {
    EXECUTABLE_TARGET = 1,
    LIBRARY_TARGET,
    CUSTOM_TARGET
};

// Key used to store the meique script object on lua registry
#define MEIQUESCRIPTOBJ_KEY "MeiqueScript"
// Key used to store the meique options, see option function
#define MEIQUEOPTIONS_KEY "MeiqueOptions"

static int findPackage(lua_State* L);
static int configureFile(lua_State* L);
static int option(lua_State* L);

extern const char meiqueApi[];

static MeiqueScript* getMeiqueScriptObject(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, MEIQUESCRIPTOBJ_KEY);
    MeiqueScript* obj = reinterpret_cast<MeiqueScript*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return obj;
}

MeiqueScript::MeiqueScript(Config& config) : m_config(config)
{
    exportApi();
    std::string scriptName = m_config.meiqueFile();
    translateLuaError(m_L, luaL_loadfile(m_L, scriptName.c_str()), scriptName);
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
    luaPCall(m_L, 0, 0, m_config.meiqueFile());

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
    translateLuaError(m_L, sanityCheck, "[meiqueApi]");
    sanityCheck = lua_pcall(m_L, 0, 0, 0);
    translateLuaError(m_L, sanityCheck, m_config.meiqueFile());

    enableBuitinScopes();

    lua_register(m_L, "findPackage", &findPackage);
    lua_register(m_L, "configureFile", &configureFile);
    lua_register(m_L, "option", &option);
    lua_settop(m_L, 0);

    // Add options table to lua registry
    lua_newtable(m_L);
    lua_setfield(m_L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);

    // Export MeiqueScript class to lua registry
    lua_pushlightuserdata(m_L, (void*) this);
    lua_setfield(m_L, LUA_REGISTRYINDEX, MEIQUESCRIPTOBJ_KEY);
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

    lua_getfield(m_L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);
    int tableIndex = lua_gettop(m_L);
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

int findPackage(lua_State* L)
{
    const char PKGCONFIG[] = "pkg-config";
    int nargs = lua_gettop(L);
    if (nargs < 1 || nargs > 3)
        LuaError(L) << "findPackage(name [, version, flags]) called with wrong number of arguments.";

    std::string pkgName = lua_tocpp<std::string>(L, 1);
    std::string version = lua_tocpp<std::string>(L, 2);
    bool optional = lua_tocpp<bool>(L, 3);

    MeiqueScript* script = getMeiqueScriptObject(L);
    Config& config = script->config();

    // do nothing when showing the help screen
    if (config.action() == Config::ShowHelp) {
        lua_settop(L, 0);
        lua_getglobal(L, "_meiqueNone");
        return 1;
    }

    // When building just check the cache for a package entry.
    StringMap pkgData;
    if (config.isInBuildMode()) {
        pkgData = config.package(pkgName);
    } else {
        // Check if the package exists
        StringList args;
        args.push_back(pkgName);
        // TODO: Interpret >, >=, < and <= from the version expression
        if (!version.empty())
            args.push_back("--atleast-version="+version);
        int retval = OS::exec(PKGCONFIG, args);
        if (retval) {
            if (!optional) {
                LuaError(L) << pkgName << " package not found!";
            } else {
                Notice() << "-- " << pkgName << red() << " not found!";
                lua_getglobal(L, "_meiqueNone");
                return 1;
            }
        } else {
            Notice() << "-- " << pkgName << green() << " found!";
        }
        args.pop_back();

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
        assert(sizeof(pkgConfigCmds) == sizeof(names));
        for (int i = 0; i < N; ++i) {
            std::string output;
            args.push_back(pkgConfigCmds[i]);
            OS::exec("pkg-config", args, &output);
            args.pop_back();
            trim(output);
            if (filters[i])
                filters[i]->filter(output);
            pkgData[names[i]] = output;
        }
        // Store pkg information
        config.setPackage(pkgName, pkgData);
    }

    if (pkgData.empty()) {
        lua_getglobal(L, "_meiqueNone");
    } else {
        lua_settop(L, 0);
        createLuaTable(L, pkgData);
        // do the magic!
        lua_getglobal(L, "_meiqueNotNone");
        lua_setmetatable(L, -2);
    }

    return 1;
}

int configureFile(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 2)
        LuaError(L) << "configureFile(input, output) called with wrong number of arguments.";

    lua_getglobal(L, "currentDir");
    lua_call(L, 0, 1);
    std::string currentDir = lua_tocpp<std::string>(L, -1) + '/';
    lua_pop(L, 1);

    // Configure anything when in the help screen
    MeiqueScript* script = getMeiqueScriptObject(L);
    if (script->config().action() == Config::ShowHelp) {
        lua_settop(L, 0);
        lua_getglobal(L, "_meiqueNotNone");
        return 1;
    }

    std::string input = script->config().sourceRoot() + currentDir + lua_tocpp<std::string>(L, -2);
    std::string output = script->config().buildRoot() + currentDir + lua_tocpp<std::string>(L, -1);

    std::ifstream in(input.c_str());
    if (!in)
        LuaError(L) << "Can't read file: " << input;
    std::ofstream out(output.c_str());

    Regex regex("@(.+)@");
    std::string line;
    while (in) {
        std::getline(in, line);
        if (regex.match(line)) {
            std::pair<int, int> idx = regex.group(1);
            lua_getglobal(L, line.substr(idx.first, idx.second) .c_str());
            std::string value = lua_tocpp<std::string>(L, -1);
            lua_pop(L, 1);
            line.replace(idx.first - 1, idx.second + 2, value);
        }
        out << line << std::endl;
    }

    return 0;
}

void MeiqueScript::enableScope(const std::string& scopeName)
{
    // Don't enable any scopes when in help screen
    if (m_config.action() == Config::ShowHelp)
        return;
    lua_State* L = luaState();
    lua_getglobal(L, "_meiqueNotNone");
    lua_setglobal(L, scopeName.c_str());
}

void MeiqueScript::enableBuitinScopes()
{
    // Don't enable any scopes when in help screen
    if (m_config.action() == Config::ShowHelp)
        return;

    StringList scopes;
    if (m_config.isInBuildMode()) {
        scopes = m_config.scopes();
    } else {
        // Enable debug/release scope
        scopes.push_back(m_config.buildType() == Config::Debug ? "DEBUG" : "RELEASE");
        // Enable compiler scope
        std::string compiler = m_config.compiler()->name();
        std::transform(compiler.begin(), compiler.end(), compiler.begin(), ::toupper);
        scopes.push_back(compiler.c_str());
        // Enable OS scopes
        StringList osVars = OS::getOSType();
        for (StringList::iterator it = osVars.begin(); it != osVars.end(); ++it)
            scopes.push_back(it->c_str());
        m_config.setScopes(scopes);
    }
    StringList::const_iterator it = scopes.begin();
    for (; it != scopes.end(); ++it)
        enableScope(*it);
}

int option(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs < 2 || nargs > 3)
        LuaError(L) << "option(name, description [, defaultValue]) called with wrong number of arguments.";
    if (nargs == 2)
        lua_pushnil(L);

    std::string name = lua_tocpp<std::string>(L, 1);
    std::string description = lua_tocpp<std::string>(L, 2);

    if (name.empty())
        LuaError(L) << "An option MUST have a name.";
    if (description.empty())
        LuaError(L) << "Be nice and put a description for the option \"" << name << "\" :-).";

    MeiqueScript* script = getMeiqueScriptObject(L);
    std::string optionValue;

    if (script->config().isInBuildMode()) {
        optionValue = script->config().userOption(name);
    } else {
        // get options table
        lua_getfield(L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);

        // Create table {description, defaultValue}
        lua_createtable(L, 2, 0);
        lua_pushvalue(L, 2);
        lua_rawseti(L, -2, 1);
        lua_pushvalue(L, 3);
        lua_rawseti(L, -2, 2);

        // to options[name] = {description, defaultValue}
        lua_setfield(L, -2, name.c_str());

        const StringMap& args = script->config().arguments();
        StringMap::const_iterator it = args.find(name);

        if (it == args.end()) {
            // option not provided by the user, uses default value.
            optionValue = lua_tocpp<std::string>(L, 3);
        } else if (it->second.empty()) {
            // option provided by the user but without a value, probably a boolean option, sets it to true.
            optionValue = "true";
        } else {
            optionValue = it->second;
        }
        lua_settop(L, 0);
    }

    // Create return object, a table with a field "value"
    lua_createtable(L, 0, 1);
    lua_pushstring(L, optionValue.c_str());
    lua_setfield(L, -2, "value");

    // Decide if the we will return a valid scope or not.
    // options setted to false
    std::transform(optionValue.begin(), optionValue.end(), optionValue.begin(), ::tolower);

    if (optionValue == "false" || optionValue == "off" || optionValue.empty()) {
        lua_getglobal(L, "_meiqueNone");
        lua_setmetatable(L, -2);
    } else {
        lua_getglobal(L, "_meiqueNotNone");
        lua_setmetatable(L, -2);
    }
    script->config().setUserOptionValue(name, optionValue);

    return 1;
}
