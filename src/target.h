/*
    This file is part of the Meique project
    Copyright (C) 2010-2013 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef TARGET_H
#define TARGET_H
#include "basictypes.h"

class MeiqueCache;
struct lua_State;
class MeiqueScript;
class Compiler;

/**
*   Base class for all meique targets.
*/
class Target
{
public:
    /// Constructs a new target with the name \p name.
    Target(const std::string& name, MeiqueScript* script);
    virtual ~Target();
    /// Returns a list with all target dependencies
    TargetList dependencies();
    /// Returns a list with all target files
    StringList files();
    /// Add files to the target
    void addFiles(const StringList& files);
    /// Get the target job queue
//    JobQueue* run(Compiler* compiler);
    /// Install all target files
    void install();
    void uninstall();
    /// Returns the target's name
    const std::string& name() const { return m_name; }
    /// Returns the target directory (relative path to source root)
    const std::string directory();
    /// Clean this target
    virtual void clean() {}
    virtual bool isCompilableTarget() const { return false; }
    bool wasRan() const { return m_ran; }
    lua_State* luaState();
    MeiqueCache* cache();
    const MeiqueScript* script() const { return m_script; }
protected:
    void getLuaField(const char* field);
    void setLuaField(const char* field);
    /// Method executed to generate the target jobs.
//    virtual JobQueue* doRun(Compiler* compiler);
    virtual void doTargetInstall(const std::string& destDir) {}
    virtual void doTargetUninstall(const std::string& destDir) {}
private:
    std::string m_name;
    bool m_ran;
    MeiqueScript* m_script;
    std::string m_directory;
    TargetList m_dependencies;
    bool m_dependenciesCached;

    Target(const Target&);
    Target& operator=(const Target&);
};

#endif
