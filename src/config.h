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

#define MEIQUECACHE "meiquecache.lua"
#define CFG_SOURCE_ROOT "sourceRoot"
#define CFG_COMPILER "compiler"
struct lua_State;

class Config
{
public:
    enum Mode {
        ConfigureMode,
        BuildMode
    };

    Config(int argc, char** argv);
    Mode mode() const { return m_mode; }

    std::string mainArgument() const { return m_mainArgument; }
    StringMap arguments() const { return m_args; }
    std::string sourceRoot() const { return m_meiqueConfig.at(CFG_SOURCE_ROOT); }
    std::string buildRoot() const { return m_buildRoot; }
    std::string compiler() const { return m_meiqueConfig.at(CFG_COMPILER); }
    void setCompiler(const std::string& name) { m_meiqueConfig[CFG_COMPILER] = name; }
    bool isInConfigureMode() const { return m_mode == ConfigureMode; }
    bool isInBuildMode() const { return m_mode == BuildMode; }
    void setUserOptions(const StringMap& userOptions);
    void saveCache();
    bool hasArgument(const std::string& arg) const;
    int jobsAtOnce() const { return m_jobsAtOnce; }

    bool isHashGroupOutdated(const StringList& files);
    void updateHashGroup(const StringList& files);
private:
    int m_jobsAtOnce;
    std::string m_mainArgument;
    StringMap m_meiqueConfig;
    std::string m_buildRoot;
    Mode m_mode;
    StringMap m_args;
    StringMap m_userOptions;
    std::map<std::string, StringMap> m_fileHashes;
    pthread_mutex_t m_configMutex;

    // disable copy
    Config(const Config&);
    Config& operator=(const Config&);
    void parseArguments(int argc, char** argv);
    void detectMode();
    void loadCache();
    static int readOption(lua_State* L);
    static int readMeiqueConfig(lua_State* L);
    static int readFileHash(lua_State* L);
};

#endif
