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

#ifndef HASHGROUPS_H
#define HASHGROUPS_H
#include "basictypes.h"
#include <pthread.h>

struct lua_State;

class HashGroups
{
public:
    HashGroups();
    void updateHashGroup(const std::string& master, const std::string& dep);
    void updateHashGroup(const std::string& master, const StringList& deps);
    bool isOutdated(const std::string& master, const std::string& dep);
    bool isOutdated(const std::string& master, const StringList& deps);
    void serializeHashGroups(std::ostream& out);
    void loadHashGroup(lua_State* L);
private:
    // {master => { master => masterhash, dep1=>hash1, dep2=>hash2, ... }}
    std::map<std::string, StringMap> m_fileHashes;
    pthread_mutex_t m_mutex;
};

#endif
