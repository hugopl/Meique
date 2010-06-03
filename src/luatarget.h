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

#ifndef LUATARGET_H
#define LUATARGET_H

#include "target.h"

class Config;
struct lua_State;
class MeiqueScript;

class LuaTarget : public Target
{
public:
    LuaTarget(const std::string& name, MeiqueScript* script);
    TargetList dependencies();
    const std::string directory();
    lua_State* luaState();
    Config& config();
    const MeiqueScript* script() const { return m_script; }
protected:
    void getLuaField(const char* field);
    JobQueue* doRun(Compiler* compiler);
private:
    std::string m_name;
    MeiqueScript* m_script;
    std::string m_directory;
    TargetList m_dependencies;
    bool m_dependenciesCached;
};

#endif // LUATARGET_H
