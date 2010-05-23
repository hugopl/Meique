/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

class Config;
class Compiler;
struct lua_State;
class MeiqueScript;

class Target
{
public:
    Target(const std::string& name, MeiqueScript* script);
    virtual ~Target();
    TargetList dependencies();
    void run(Compiler* compiler);
    lua_State* luaState();
    Config& config();
    const std::string& name() const { return m_name; }
    const std::string& directory();
    virtual void clean() {}
protected:
    virtual void doRun(Compiler* compiler);
    void getLuaField(const char* field);
private:
    std::string m_name;
    std::string m_directory;
    MeiqueScript* m_script;
    TargetList m_dependencies;
    bool m_dependenciesCached;

    Target(const Target&);
    Target operator=(const Target&);
};

#endif
