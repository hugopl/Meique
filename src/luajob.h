/*
    This file is part of the Meique project
    Copyright (C) 2010-2014 Hugo Parente Lima <hugo.pl@gmail.com>

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

#ifndef LUAJOB_H
#define LUAJOB_H
#include "job.h"

struct lua_State;
class LuaJob : public Job
{
public:
    /// Pops a lua_function from stack, store in the lua registry and execute them later
    LuaJob(NodeGuard* nodeGuard, lua_State* L, int args);
protected:
    virtual int doRun();
private:
    lua_State* m_L;
};

#endif // LUAJOB_H
