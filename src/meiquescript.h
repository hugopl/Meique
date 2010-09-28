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

#ifndef MEIQUESCRIPT_H
#define MEIQUESCRIPT_H

#include <string>
#include "basictypes.h"
#include "luastate.h"
#include "meiqueoption.h"
#include <list>

class Target;
class Config;

class MeiqueScript
{
public:
    MeiqueScript(Config& config);
    ~MeiqueScript();
    void exec();
    OptionsMap options();
    Config& config() { return m_config; }
    Target* getTarget(const std::string& name) const;
    TargetList targets() const;
    lua_State* luaState() { return m_L; }

private:
    LuaState m_L;
    OptionsMap m_options;
    TargetsMap m_targets;
    Config& m_config;

    // disable copy
    MeiqueScript(const MeiqueScript&);
    MeiqueScript& operator=(const MeiqueScript&);

    void exportApi();
    void extractTargets();

    void enableScope(const std::string& scopeName);
    void enableBuitinScopes();
};

#endif
