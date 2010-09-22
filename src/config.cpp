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

#include "config.h"
#include "logger.h"
#include "luastate.h"
#include "luacpputil.h"
#include "stdstringsux.h"
#include <fstream>
#include <sstream>

#include "lauxlib.h"

#include "os.h"
#include "mutexlocker.h"
#include "filehash.h"
#include "compilerfactory.h"
#include "compiler.h"

int verboseMode = 0;

Config::Config(int argc, char** argv) : m_jobsAtOnce(1), m_compiler(0)
{
    pthread_mutex_init(&m_configMutex, 0);
    m_meiqueConfig[CFG_BUILD_TYPE] = "release"; // default value for build type is release.
    detectMode();
    parseArguments(argc, argv);
    std::string verboseValue = OS::getEnv("VERBOSE");
    std::istringstream s(verboseValue);
    s >> verboseMode;

    if (mode() == BuildMode) {
        m_buildRoot = OS::pwd();
        std::string compilerName = m_meiqueConfig[CFG_COMPILER];
        if (compilerName.empty())
            Error() << "Unable to find the compiler entry on your meiquecache.lua.";
        m_compiler = CompilerFactory::createCompiler(compilerName);
        if (!m_compiler)
            Error() << "The compiler '" << compilerName << "' doesn't exists!";
    } else {
        m_compiler = CompilerFactory::findCompiler();
        m_meiqueConfig[CFG_COMPILER] = m_compiler->name();
    }
}

Config::~Config()
{
    delete m_compiler;
}

void Config::parseArguments(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.find("-j") == 0) {
            arg.erase(0, 2);
            std::istringstream s(arg);
            s >> m_jobsAtOnce;
            if (m_jobsAtOnce < 1)
                Error() << "You must use a number greater than 1 with -j option.";
        } else if (arg.find("--") != 0) { // non-option, must be the source directory
            if (m_mainArgument.size()) {
                // TODO: A better error message
                Error() << "The main argument was already specified [main argument: " << m_mainArgument << "].";
            } else {
                m_mainArgument = arg;
            }
        } else if (m_mode == BuildMode && arg != "--help" && arg != "--version") {
            Error() << "You can use option \"" << arg << "\" only when configuring the project.";
        } else{
            arg.erase(0, 2);
            if (arg == "debug") {
                m_meiqueConfig[CFG_BUILD_TYPE] = "debug";
            } else if (arg == "release") {
                m_meiqueConfig[CFG_BUILD_TYPE] = "release";
            } else {
                size_t equalPos = arg.find("=");
                if (equalPos == std::string::npos)
                    m_args[arg] = std::string();
                else
                    m_args[arg.substr(0, equalPos)] = arg.substr(equalPos + 1, arg.size() - equalPos);
            }
        }
    }

    if (m_mode == ConfigureMode) {
        if (!m_mainArgument.empty()) {
            if (m_mainArgument[0] != '/')
                m_mainArgument = OS::pwd() + m_mainArgument;
            if (*(--m_mainArgument.end()) != '/')
                m_mainArgument += '/';
            m_meiqueConfig[CFG_SOURCE_ROOT] = m_mainArgument;
        } else {
            m_meiqueConfig[CFG_SOURCE_ROOT] = OS::pwd();
        }
    }
}

void Config::detectMode()
{
    std::ifstream file(MEIQUECACHE);
    if (file) {
        file.close();
        m_mode = BuildMode;
        loadCache();
        // check if the key attributes are present
        const char* attrs[] = { CFG_SOURCE_ROOT, CFG_COMPILER, 0 };
        for (int i = 0; attrs[i]; ++i)
            if (m_meiqueConfig.find(attrs[i]) == m_meiqueConfig.end())
                Error() << "meiquecache.lua is probably corrupted, value for key \"" << attrs[i] << "\" was not found!";
    } else {
        m_mode = ConfigureMode;
    }
}

void Config::loadCache()
{
    LuaState L;
    lua_register(L, "userOption", &readOption);
    lua_register(L, "meiqueConfig", &readMeiqueConfig);
    lua_register(L, "fileHash", &readFileHash);
    lua_register(L, "package", &readPackage);
    lua_register(L, "scopes", &readScopes);
    // put a pointer to this instance of Config in lua registry, the key is the L address.
    lua_pushlightuserdata(L, (void *)L);
    lua_pushlightuserdata(L, (void *)this);
    lua_settable(L, LUA_REGISTRYINDEX);

    int res = luaL_loadfile(L, MEIQUECACHE);
    if (res)
        Error() << "Error loading " MEIQUECACHE ", this *should* never happen. A bug? maybe...";
    if (lua_pcall(L, 0, 0, 0))
        Error() << "Error loading " MEIQUECACHE ": " << lua_tostring(L, -1);
}

// Retrieve the Config instance
static Config* getSelf(lua_State* L)
{
    lua_pushlightuserdata(L, (void *)L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Config* self = reinterpret_cast<Config*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return self;
}

void Config::saveCache()
{
    std::ofstream file(MEIQUECACHE);
    if (!file.is_open())
        Error() << "Can't open " MEIQUECACHE " for write.";
    StringMap::const_iterator mapIt = m_userOptions.begin();
    for (; mapIt != m_userOptions.end(); ++mapIt) {
        std::string name(mapIt->first);
        stringReplace(name, "\"", "\\\"");
        std::string value(mapIt->second);
        stringReplace(name, "\"", "\\\"");
        file << "userOption {\n"
                "    name = \"" << name << "\",\n"
                "    value = \"" << value << "\"\n"
                "}\n\n";
    }

    file << "meiqueConfig {\n";
    mapIt = m_meiqueConfig.begin();
    for (; mapIt != m_meiqueConfig.end(); ++mapIt) {
        std::string value = mapIt->second;
        stringReplace(value, "\"", "\\\"");
        file << "    " << mapIt->first << " = \"" << value << "\",\n";
    }
    file << "}\n\n";

    // Cached scopes
    file << "scopes {\n";
    StringList::const_iterator listIt = m_scopes.begin();
    for (; listIt != m_scopes.end(); ++listIt) {
        file << "    \"" << *listIt << "\"," << std::endl;
    }
    file << "}\n\n";


    // Info about packages
    std::map<std::string, StringMap>::iterator mapMapIt = m_packages.begin();
    for (; mapMapIt != m_packages.end(); ++mapMapIt) {
        file << "package {\n";
        std::string name(mapMapIt->first);
        stringReplace(name, "\"", "\\\"");
        file << "    name = \"" << name << "\",\n";
        // Write package data
        // Write other files hashes
        StringMap::const_iterator it = mapMapIt->second.begin();
        for (; it != mapMapIt->second.end(); ++it) {
            std::string value(it->second);
            stringReplace(value, "\"", "\\\"");
            file << "    " << it->first << " = \"" << value << "\",\n";
        }
        file << "}\n\n";
    }

    mapMapIt = m_fileHashes.begin();
    for (; mapMapIt != m_fileHashes.end(); ++mapMapIt) {
        if (mapMapIt->second.empty())
            continue;

        file << "fileHash {\n";
        // Write the key first!
        std::string name(mapMapIt->first);
        stringReplace(name, "\"", "\\\"");
        file << "    \"" << name << "\",\n";
        file << "    \"" << mapMapIt->second[name] << "\",\n";

        // Write other files hashes
        StringMap::const_iterator it = mapMapIt->second.begin();
        for (; it != mapMapIt->second.end(); ++it) {
            // Skip the file hash if it's the key file!
            if (it->first == mapMapIt->first || it->first.empty())
                continue;

            name = it->first;
            stringReplace(name, "\"", "\\\"");
            file << "    \"" << name << "\",\n";

            std::string value(it->second);
            file << "    \"" << value << "\",\n";
        }
        file << "}\n\n";
    }
}

bool Config::hasArgument(const std::string& arg) const
{
    StringMap::const_iterator it = m_args.find(arg);
    return it != m_args.end();
}

int Config::readOption(lua_State* L)
{
    Config* self = getSelf(L);
    std::string name = getField<std::string>(L, "name");
    std::string value = getField<std::string>(L, "value");
    self->m_userOptions[name] = value;
    return 0;
}

int Config::readMeiqueConfig(lua_State* L)
{
    Config* self = getSelf(L);
    readLuaTable(L, lua_gettop(L), self->m_meiqueConfig);
    return 0;
}

int Config::readFileHash(lua_State* L)
{
    Config* self = getSelf(L);
    StringList list;
    readLuaList(L, 1, list);

    if (list.empty() || list.size() % 2)
        LuaError(L) << "File hash database corrupted!";

    StringList::const_iterator it = list.begin();
    StringMap& map = self->m_fileHashes[*it];
    for (; it != list.end(); ++it) {
        std::string name = *it;
        std::string value = *(++it);
        map[name] = value;
    }
    return 0;
}

bool Config::isHashGroupOutdated(const StringList& files)
{
    MutexLocker locker(&m_configMutex);
    if (files.empty())
        return false;
    StringList::const_iterator it = files.begin();
    StringMap& map = m_fileHashes[files.front()];
    if (map.empty())
        return true;
    for (; it != files.end(); ++it) {
        if (getFileHash(*it) != map[*it])
            return true;
    }
    return false;
}

void Config::updateHashGroup(const StringList& files)
{
    MutexLocker locker(&m_configMutex);
    if (files.empty())
        return;

    StringList::const_iterator it = files.begin();
    StringMap& map = m_fileHashes[files.front()];
    for (; it != files.end(); ++it)
        map[*it] = getFileHash(*it);
}

void Config::setUserOptions(const StringMap& userOptions)
{
    m_userOptions = userOptions;
}

Config::BuildType Config::buildType() const
{
    const std::string& value = m_meiqueConfig.at(CFG_BUILD_TYPE);
    if (value == "release")
        return Release;
    else if (value == "debug")
        return Debug;
    Warn() << "Unknown build type, using \"release\".";
    return Release;
}

StringMap Config::package(const std::string& pkgName) const
{
    std::map<std::string, StringMap>::const_iterator it = m_packages.find(pkgName);
    if (it != m_packages.end())
        return it->second;
    return StringMap();
}

void Config::setPackage(const std::string& pkgName, const StringMap& pkgData)
{
    m_packages[pkgName] = pkgData;
}

int Config::readPackage(lua_State* L)
{
    StringMap pkgData;
    readLuaTable(L, lua_gettop(L), pkgData);
    std::string name = pkgData["name"];
    if (name.empty())
        LuaError(L) << "Package entry without name.";
    Config* self = getSelf(L);
    self->setPackage(name, pkgData);
    return 0;
}

StringList Config::scopes() const
{
    return m_scopes;
}

void Config::setScopes(const StringList& scopes)
{
    m_scopes = scopes;
}

int Config::readScopes(lua_State* L)
{
    Config* self = getSelf(L);
    readLuaList(L, 1, self->m_scopes);
    return 0;
}

