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

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

#include "os.h"
#include "mutexlocker.h"

int verboseMode = 0;

Config::Config(int argc, char** argv) : m_jobsAtOnce(1)
{
    pthread_mutex_init(&m_configMutex, 0);
    detectMode();
    parseArguments(argc, argv);
    std::string verboseValue = OS::getEnv("VERBOSE");
    std::istringstream s(verboseValue);
    s >> verboseMode;
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
        } else {
            arg.erase(0, 2);
            size_t equalPos = arg.find("=");
            if (equalPos == std::string::npos)
                m_args[arg] = std::string();
            else
                m_args[arg.substr(0, equalPos)] = arg.substr(equalPos + 1, arg.size() - equalPos);
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
    // put a pointer to this instance of Config in lua registry, the key is the L address.
    lua_pushlightuserdata(L, (void *)L);
    lua_pushlightuserdata(L, (void *)this);
    lua_settable(L, LUA_REGISTRYINDEX);

    int res = luaL_loadfile(L, MEIQUECACHE);
    if (res)
        Error() << "Error loading " MEIQUECACHE ", this *should* never happen. A bug? maybe...";
    if (lua_pcall(L, 0, 0, 0))
        Error() << "Error loading " MEIQUECACHE ".";
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
    StringMap::const_iterator it = m_userOptions.begin();
    for (; it != m_userOptions.end(); ++it) {
        std::string name(it->first);
        stringReplace(name, "\"", "\\\"");
        std::string value(it->second);
        stringReplace(name, "\"", "\\\"");
        file << "userOption {\n"
                "    name = \"" << name << "\",\n"
                "    value = \"" << value << "\"\n"
                "}\n\n";
    }

    file << "meiqueConfig {\n";
    it = m_meiqueConfig.begin();
    for (; it != m_meiqueConfig.end(); ++it) {
        std::string value = it->second;
        stringReplace(value, "\"", "\\\"");
        file << "    " << it->first << " = \"" << value << "\",\n";
    }
    file << "}\n\n";

    it = m_fileHashes.begin();
    for (; it != m_fileHashes.end(); ++it) {
        std::string name(it->first);
        stringReplace(name, "\"", "\\\"");
        std::string value(it->second);
        file << "fileHash {\n"
                "    file = \"" << name << "\",\n"
                "    hash = \"" << value << "\"\n"
                "}\n\n";
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
    std::string name = getField<std::string>(L, "file");
    std::string value = getField<std::string>(L, "hash");
    self->m_fileHashes[name] = value;
    return 0;
}

void Config::setFileHash(const std::string& fileName, const std::string& hash)
{
    MutexLocker locker(&m_configMutex);
    if (!fileName.empty() && !hash.empty())
        m_fileHashes[fileName] = hash;
}

std::string Config::fileHash(const std::string& fileName) const
{
    StringMap::const_iterator it = m_fileHashes.find(fileName);
    if (it != m_fileHashes.end())
        return it->second;
    return std::string();
}

void Config::setUserOptions(const StringMap& userOptions)
{
    m_userOptions = userOptions;
}
