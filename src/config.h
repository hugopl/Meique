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

#ifndef CONFIG_H
#define CONFIG_H

#include "basictypes.h"
#include "meiqueoption.h"
#include "hashgroups.h"

class CmdLine;
struct lua_State;
class Compiler;

class MeiqueCache
{
public:
    enum BuildType {
        NoType,
        Debug,
        Release
    };

    /// Build a cache based on a meiquecache.lua file at the current directory.
    MeiqueCache();
    /// Create a new cache based somehow on the given command line
    MeiqueCache(const CmdLine* cmdLine);
    ~MeiqueCache();

    Compiler* compiler();

    void setUserOptionValue(const std::string& key, const std::string& value);
    std::string userOption(const std::string& key) const;

    void saveCache();

    BuildType buildType() const { return m_buildType; }

    bool isHashGroupOutdated(const std::string& masterFile, const std::string& dep = std::string());
    bool isHashGroupOutdated(const std::string& masterFile, const StringList& deps);
    void updateHashGroup(const std::string& masterFile, const std::string& dep);
    void updateHashGroup(const std::string& masterFile, const StringList& deps = StringList());

    StringMap package(const std::string& pkgName) const;
    void setPackage(const std::string& pkgName, const StringMap& pkgData);
    StringList scopes() const;
    void setScopes(const StringList& scopes);

    void setSourceDir(const std::string& dir) { m_sourceDir = dir; }
    std::string sourceDir() const { return m_sourceDir; }
private:
    // Arguments
    BuildType m_buildType;

    // Env. stuff
    std::string m_compilerName;
    Compiler* m_compiler;

    std::string m_sourceDir;

    // Stuff stored in meiquecache.lua by meique.lua
    std::map<std::string, StringMap> m_packages;
    HashGroups m_hashGroups;
    StringList m_scopes;
    StringList m_targets;
    StringMap m_userOptions;

    // helper variables
    pthread_mutex_t m_configMutex;

    void loadCache();
    static int readOption(lua_State* L);
    static int readMeiqueConfig(lua_State* L);
    static int readHashGroup(lua_State* L);
    static int readPackage(lua_State* L);
    static int readScopes(lua_State* L);

    // disable copy
    MeiqueCache(const MeiqueCache&);
    MeiqueCache& operator=(const MeiqueCache&);
    void init();
};

#endif
