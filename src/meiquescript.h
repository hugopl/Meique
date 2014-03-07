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

#ifndef MEIQUESCRIPT_H
#define MEIQUESCRIPT_H

#include <string>
#include <list>
#include "basictypes.h"
#include "luastate.h"

class CmdLine;
class MeiqueCache;

class MeiqueScript
{
public:
    MeiqueScript();
    MeiqueScript(const std::string scriptName, const CmdLine* cmdLine);
    ~MeiqueScript();
    void exec();

    MeiqueCache* cache() { return m_cache; }
    StringList targetNames();
    StringMap getOptionsValues();
    lua_State* luaState() { return m_L; }

    std::list<StringList> getTests(const std::string& pattern);

    void setSourceDir(const std::string& sourceDir);
    void setBuildDir(const std::string& buildDir);
    std::string sourceDir() const;
    std::string buildDir() const { return m_buildDir; }

    const CmdLine* commandLine() const { return m_cmdLine; }

    StringList projectFiles();

    void luaPushTarget(const char* target);
    void luaPushTarget(const std::string& target) { luaPushTarget(target.c_str()); }

    void installTargets(const StringList& targets);
    void uninstallTargets(const StringList& targets);
    void cleanTargets(const StringList& targets);

    void dumpProject(std::ostream& output);

    StringList getTargetIncludeDirectories(const std::string& target);
    bool hasHook(const char* target);
private:
    LuaState m_L;
    MeiqueCache* m_cache;

    std::string m_scriptName;
    std::string m_buildDir;

    const CmdLine* m_cmdLine;

    void populateOptionsValues();

    // disable copy
    MeiqueScript(const MeiqueScript&) = delete;
    MeiqueScript& operator=(const MeiqueScript&) = delete;

    void exportApi();

    void enableScope(const std::string& scopeName);
    void enableBuitinScopes();
};

#endif
