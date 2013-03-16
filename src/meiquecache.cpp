/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "meiquecache.h"
#include "logger.h"
#include "luacpputil.h"
#include "stdstringsux.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

#include "lauxlib.h"

#include "os.h"
#include "compilerfactory.h"
#include "compiler.h"

#define MEIQUECACHE "meiquecache.lua"
#define CFG_SOURCE_DIR "sourceDir"
#define CFG_COMPILER "compiler"
#define CFG_BUILD_TYPE "buildType"
#define CFG_INSTALL_PREFIX "installPrefix"

/*
 * We need to save the cache when the user hits CTRL+C.
 *
 * Note: If there are two instances of MeiqueCache, only the last one
 *       will have the cache saved!
 */
MeiqueCache* currentCache = 0;
void handleCtrlC()
{
    if (currentCache && currentCache->isAutoSaveEnabled())
        currentCache->saveCache();
    std::exit(1);
}

MeiqueCache::MeiqueCache()
{
    assert(!currentCache);
    currentCache = this;
    OS::setCTRLCHandler(handleCtrlC);

    m_compiler = 0;
    m_autoSave = true;
}

MeiqueCache::~MeiqueCache()
{
    if (m_autoSave)
        saveCache();
    delete m_compiler;
}

Compiler* MeiqueCache::compiler()
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

void MeiqueCache::loadCache()
{
    lua_State* L = luaL_newstate();
    lua_register(L, "userOption", &readOption);
    lua_register(L, "meiqueConfig", &readMeiqueConfig);
    lua_register(L, "package", &readPackage);
    lua_register(L, "scopes", &readScopes);
    lua_register(L, "TargetHash", &readTargetHash);
    // put a pointer to this instance of Config in lua registry, the key is the L address.
    lua_pushlightuserdata(L, (void *)L);
    lua_pushlightuserdata(L, (void *)this);
    lua_settable(L, LUA_REGISTRYINDEX);

    int res = luaL_loadfile(L, MEIQUECACHE);
    if (res)
        throw Error("Error loading " MEIQUECACHE ", this *should* never happen. A bug? maybe...");
    if (lua_pcall(L, 0, 0, 0))
        throw Error("Error loading " MEIQUECACHE ": " + std::string(lua_tostring(L, -1)));
}

// Retrieve the Config instance
static MeiqueCache* getSelf(lua_State* L)
{
    lua_pushlightuserdata(L, (void *)L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    MeiqueCache* self = reinterpret_cast<MeiqueCache*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return self;
}

void MeiqueCache::saveCache()
{
    std::ofstream file(MEIQUECACHE);
    if (!file.is_open())
        throw Error("Can't open " MEIQUECACHE " for write.");

    StringMap::const_iterator mapIt = m_userOptions.begin();
    for (; mapIt != m_userOptions.end(); ++mapIt) {
        std::string name(mapIt->first);
        if (name.empty())
            continue; // Default package doesn't need to be saved.
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
    file << "    " CFG_SOURCE_DIR " = \"" << m_sourceDir << "\",\n";
    if (!m_installPrefix.empty())
        file << "    " CFG_INSTALL_PREFIX " = \"" << m_installPrefix << "\",\n";
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
        StringMap::const_iterator it = mapMapIt->second.begin();
        for (; it != mapMapIt->second.end(); ++it) {
            std::string value(it->second);
            stringReplace(value, "\"", "\\\"");
            file << "    " << it->first << " = \"" << value << "\",\n";
        }
        file << "}\n\n";
    }

    // target hashes
    for (auto& pair : m_targetHashes) {
        file << "TargetHash {\n"
                "    target = \"" << escape(pair.first) << "\",\n";
        file << "    hash = \"" << escape(pair.second) << "\"\n";
        file << "}\n\n";
    }
}

int MeiqueCache::readOption(lua_State* L)
{
    MeiqueCache* self = getSelf(L);
    std::string name = getField<std::string>(L, "name");
    std::string value = getField<std::string>(L, "value");
    self->m_userOptions[name] = value;
    return 0;
}

int MeiqueCache::readMeiqueConfig(lua_State* L)
{
    MeiqueCache* self = getSelf(L);
    StringMap opts;
    readLuaTable(L, lua_gettop(L), opts);
    lua_pop(L, 1);
    try {
        self->m_sourceDir = OS::normalizeDirPath(opts.at(CFG_SOURCE_DIR));
        self->m_buildType = opts.at(CFG_BUILD_TYPE) == "debug" ? Debug : Release;
        self->m_compilerName = opts.at(CFG_COMPILER);
        self->m_installPrefix = opts[CFG_INSTALL_PREFIX];
    } catch (std::out_of_range&) {
        throw Error(MEIQUECACHE " file corrupted, some fundamental entry is missing.");
    }
    return 0;
}

int MeiqueCache::readTargetHash(lua_State* L)
{
    LuaLeakCheck(L);
    MeiqueCache* self = getSelf(L);
    std::string target = getField<std::string>(L, "target");
    std::string hash = getField<std::string>(L, "hash");
    self->m_targetHashes[target] = hash;
    return 0;
}

StringMap MeiqueCache::package(const std::string& pkgName) const
{
    std::map<std::string, StringMap>::const_iterator it = m_packages.find(pkgName);
    if (it != m_packages.end())
        return it->second;
    return StringMap();
}

void MeiqueCache::setPackage(const std::string& pkgName, const StringMap& pkgData)
{
    m_packages[pkgName] = pkgData;
}

int MeiqueCache::readPackage(lua_State* L)
{
    StringMap pkgData;
    readLuaTable(L, lua_gettop(L), pkgData);
    lua_pop(L, 1);
    std::string name = pkgData["name"];
    if (name.empty())
        luaError(L, "Package entry without name.");
    MeiqueCache* self = getSelf(L);
    self->setPackage(name, pkgData);
    return 0;
}

StringList MeiqueCache::scopes() const
{
    return m_scopes;
}

void MeiqueCache::setScopes(const StringList& scopes)
{
    m_scopes = scopes;
}

int MeiqueCache::readScopes(lua_State* L)
{
    MeiqueCache* self = getSelf(L);
    readLuaList(L, 1, self->m_scopes);
    lua_pop(L, 1);
    return 0;
}

void MeiqueCache::setSourceDir(const std::string& dir)
{
    m_sourceDir = OS::normalizeDirPath(dir);
}

std::string MeiqueCache::installPrefix()
{
    // Check if DESTDIR env variable is set
    std::string destDir = OS::getEnv("DESTDIR");
    if (!destDir.empty())
        return OS::normalizeDirPath(destDir);

    // Last resort, the default install prefix.
    if (m_installPrefix.empty())
        return  OS::defaultInstallPrefix();
    return m_installPrefix;
}

std::string MeiqueCache::targetHash(const std::string& target) const
{
    auto it = m_targetHashes.find(target);
    return it != m_targetHashes.end() ? it->second : std::string();
}
