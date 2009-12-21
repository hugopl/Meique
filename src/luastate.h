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

#ifndef LUASTATE_H
#define LUASTATE_H

struct lua_State;

/// RAII for lua state
class LuaState {
public:
    LuaState();
    ~LuaState();
    operator lua_State*() { return m_L; }
private:
    lua_State* m_L;

    LuaState(const LuaState&);
    LuaState& operator=(const LuaState&);
};

#endif
