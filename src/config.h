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

struct lua_State;
class Compiler;

class Config
{
public:
    enum Action {
        NoAction,
        ShowHelp,
        ShowVersion,
        Build,
        Install,
        Clean,
        Uninstall
    };

    enum BuildType {
        NoType,
        Debug,
        Release
    };

    enum Mode {
        NoMode,
        ConfigureMode,
        BuildMode
    };

    Config(int argc, char** argv);
    ~Config();

    StringMap arguments() const { return m_args; }
    StringList targets() const { return m_targets; }
    std::string meiqueFile() const;

    Action action() const { return m_action; }

    std::string sourceRoot() const { return m_sourceRoot; }
    std::string buildRoot() const { return m_buildRoot; }

    Compiler* compiler();

    bool isInConfigureMode() const { return m_mode == ConfigureMode; }
    bool isInBuildMode() const { return m_mode == BuildMode; }

    void setUserOptionValue(const std::string& key, const std::string& value);
    std::string userOption(const std::string& key) const;

    void saveCache();

    int jobsAtOnce() const { return m_jobsAtOnce; }

    BuildType buildType() const { return m_buildType; }

    bool isHashGroupOutdated(const std::string& masterFile, const std::string& dep = std::string());
    bool isHashGroupOutdated(const std::string& masterFile, const StringList& deps);
    void updateHashGroup(const std::string& masterFile, const StringList& deps = StringList());

    StringMap package(const std::string& pkgName) const;
    void setPackage(const std::string& pkgName, const StringMap& pkgData);
    StringList scopes() const;
    void setScopes(const StringList& scopes);
private:
    // Arguments
    int m_jobsAtOnce;
    Mode m_mode;
    Action m_action;
    BuildType m_buildType;
    StringMap m_args;

    // Env. stuff
    std::string m_buildRoot;
    std::string m_sourceRoot;
    std::string m_compilerName;
    Compiler* m_compiler;

    // Stuff stored in meiquecache.lua by meique.lua
    std::map<std::string, StringMap> m_packages;
    HashGroups m_hashGroups;
    StringList m_scopes;
    StringList m_targets;
    StringMap m_userOptions;

    // helper variables
    pthread_mutex_t m_configMutex;

    // disable copy
    Config(const Config&);
    Config& operator=(const Config&);
    void parseArguments(int argc, char** argv);
    void setAction(const Config::Action& action);
    void detectMode();
    void loadCache();
    static int readOption(lua_State* L);
    static int readMeiqueConfig(lua_State* L);
    static int readHashGroup(lua_State* L);
    static int readPackage(lua_State* L);
    static int readScopes(lua_State* L);
    void setBuildMode(const Config::BuildType& mode);
};

#endif
