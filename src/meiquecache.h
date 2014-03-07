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

#ifndef MEIQUECACHE_H
#define MEIQUECACHE_H

#include "basictypes.h"

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

    MeiqueCache();
    ~MeiqueCache();

    Compiler* compiler();

    void setUserOptionsValues(const StringMap& options) { m_userOptions = options; }
    const StringMap& userOptionsValues() const { return m_userOptions; }

    void saveCache();
    void loadCache();

    void setBuildType(BuildType value) { m_buildType = value; }
    BuildType buildType() const { return m_buildType; }

    StringMap package(const std::string& pkgName) const;
    void setPackage(const std::string& pkgName, const StringMap& pkgData);
    StringList scopes() const;
    void setScopes(const StringList& scopes);

    void setSourceDir(const std::string& dir);
    std::string sourceDir() const { return m_sourceDir; }

    void setAutoSave(bool value) { m_autoSave = value; }
    bool isAutoSaveEnabled() const { return m_autoSave; }

    void setInstallPrefix(const std::string& value) { m_installPrefix = value; }
    std::string installPrefix();
private:
    // Arguments
    BuildType m_buildType;

    // Env. stuff
    std::string m_compilerName;
    Compiler* m_compiler;

    std::string m_sourceDir;

    // Stuff stored in meiquecache.lua by meique.lua
    std::map<std::string, StringMap> m_packages;
    StringList m_scopes;
    StringList m_targets;
    StringMap m_userOptions;
    std::string m_installPrefix;

    // helper variables
    bool m_autoSave;

    static int readOption(lua_State* L);
    static int readMeiqueConfig(lua_State* L);
    static int readPackage(lua_State* L);
    static int readScopes(lua_State* L);

    // disable copy
    MeiqueCache(const MeiqueCache&);
    MeiqueCache& operator=(const MeiqueCache&);
};

#endif
