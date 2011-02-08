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
#include <stdexcept>

#include "lauxlib.h"

#include "os.h"
#include "mutexlocker.h"
#include "filehash.h"
#include "compilerfactory.h"
#include "compiler.h"

#define MEIQUECACHE "meiquecache.lua"
#define CFG_SOURCE_ROOT "sourceRoot"
#define CFG_COMPILER "compiler"
#define CFG_BUILD_TYPE "buildType"

int verboseMode = 0;

Config::Config(int argc, char** argv) : m_jobsAtOnce(1), m_action(NoAction), m_compiler(0)
{
    pthread_mutex_init(&m_configMutex, 0);
    parseArguments(argc, argv);
    if (m_buildType == NoType)
        m_buildType = Release;

    std::string verboseValue = OS::getEnv("VERBOSE");
    std::istringstream s(verboseValue);
    s >> verboseMode;

    if (m_mode == BuildMode)
        m_buildRoot = OS::pwd();
}

Config::~Config()
{
    if (isInBuildMode() || (isInConfigureMode() && m_action != ShowHelp))
        saveCache();
    delete m_compiler;
}

std::string Config::meiqueFile() const
{
    return m_sourceRoot + "meique.lua";
}

Compiler* Config::compiler()
{
    if (!m_compiler) {
        if (m_compilerName.empty()) {
            m_compiler = CompilerFactory::findCompiler();
            m_compilerName = m_compiler->name();
        } else {
            m_compiler = CompilerFactory::createCompiler(m_compilerName);
        }
    }
    return m_compiler;
}

void Config::setAction(const Config::Action& action)
{
    if (m_action != Config::NoAction)
        Error() << "The options u, i, c, help and version are mutually exclusives!";
    m_action = action;
}

void Config::setBuildMode(const Config::BuildType& mode)
{
    if (m_buildType != Config::NoType)
        Error() << "The options release and debug are mutually exclusives!";
    m_buildType = mode;

}

void Config::parseArguments(int argc, char** argv)
{
    detectMode();

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        // long options
        if (arg.find("--") == 0) {
            arg.erase(0, 2);
            if (arg == "version") {
                setAction(ShowVersion);
            } else if (arg == "help") {
                setAction(ShowHelp);
            } else if (arg == "debug") {
                setBuildMode(Debug);
            } else if (arg == "release") {
                setBuildMode(Release);
            } else {
                size_t equalPos = arg.find("=");
                if (equalPos == std::string::npos)
                    m_args[arg] = std::string();
                else
                    m_args[arg.substr(0, equalPos)] = arg.substr(equalPos + 1, arg.size() - equalPos);
            }
        } else if (arg[0] == '-') { // short options
            arg.erase(0, 1);
            while (!arg.empty()) {
                const char c = arg[0];
                arg.erase(0, 1);
                if (c == 'j') {
                    std::istringstream s(arg);
                    s >> m_jobsAtOnce;
                    if (m_jobsAtOnce < 1)
                        Error() << "You must use a number greater than 1 with -j option.";
                    arg.erase(0, s.tellg());
                } else if (c == 'i') {
                    setAction(Install);
                } else if (c == 'u') {
                    setAction(Uninstall);
                } else if (c == 'c') {
                    setAction(Clean);
                } else if (c == 't') {
                    setAction(Test);
                } else {
                    Error() << "Unknown option '-" << c << "', type meique --help to see the available options.";
                }
            }
        } else if (m_mode == BuildMode) {
            m_targets.push_back(arg);
        } else if (m_mode == ConfigureMode) {
            if (!m_sourceRoot.empty())
                Error() << "Two meique.lua directories specified! " << m_sourceRoot << " and " << arg << '.';
            m_sourceRoot = arg;
        }
    }

    if (m_mode == ConfigureMode) {
        if (!m_sourceRoot.empty()) {
            if (m_sourceRoot[0] != '/')
                m_sourceRoot = OS::pwd() + m_sourceRoot;
            if (*(--m_sourceRoot.end()) != '/')
                m_sourceRoot += '/';
        } else if (m_action != ShowHelp && m_action != ShowVersion) {
            Error() << "Non out of source build detected! It's ugly, dirty and a bad habbit! If you can't avoid it we will force you to do so!";
        }
    } else if (m_action == NoAction) {
        m_action = Build;
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
    lua_register(L, "hashGroup", &readHashGroup);
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
    file << "    " CFG_BUILD_TYPE " = \"" << (m_buildType == Debug ? "debug" : "release") << "\",\n";
    file << "    " CFG_COMPILER " = \"" << m_compilerName << "\",\n";
    file << "    " CFG_SOURCE_ROOT " = \"" << m_sourceRoot << "\",\n";
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
    m_hashGroups.serializeHashGroups(file);
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
    StringMap opts;
    readLuaTable(L, lua_gettop(L), opts);
    try {
        self->m_sourceRoot = opts.at(CFG_SOURCE_ROOT);
        self->m_buildType = opts.at(CFG_BUILD_TYPE) == "debug" ? Debug : Release;
        self->m_compilerName = opts.at(CFG_COMPILER);
    } catch (std::out_of_range& e) {
        Error() << MEIQUECACHE " file corrupted, some fundamental entry is missing.";
    }
    return 0;
}

int Config::readHashGroup(lua_State* L)
{
    Config* self = getSelf(L);
    self->m_hashGroups.loadHashGroup(L);
    return 0;
}

bool Config::isHashGroupOutdated(const std::string& masterFile, const std::string& dep)
{
    return m_hashGroups.isOutdated(masterFile, dep);
}

bool Config::isHashGroupOutdated(const std::string& masterFile, const StringList& deps)
{
    return m_hashGroups.isOutdated(masterFile, deps);
}

void Config::updateHashGroup(const std::string& masterFile, const std::string& dep)
{
    m_hashGroups.updateHashGroup(masterFile, dep);
}

void Config::updateHashGroup(const std::string& masterFile, const StringList& deps)
{
    m_hashGroups.updateHashGroup(masterFile, deps);
}

void Config::setUserOptionValue(const std::string& key, const std::string& value)
{
    m_userOptions[key] = value;
}

std::string Config::userOption(const std::string& key) const
{
    StringMap::const_iterator it = m_userOptions.find(key);
    return it == m_userOptions.end() ? std::string() : it->second;
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

