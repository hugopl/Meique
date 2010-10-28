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
#include "hashgroups.h"
#include "mutexlocker.h"
#include "filehash.h"
#include "stdstringsux.h"
#include "luacpputil.h"
#include "logger.h"

HashGroups::HashGroups()
{
    pthread_mutex_init(&m_mutex, 0);
}

bool HashGroups::isOutdated(const std::string& master, const std::string& dep)
{
    StringList deps;
    if (!dep.empty())
        deps.push_back(dep);
    return isOutdated(master, deps);
}

bool HashGroups::isOutdated(const std::string& master, const StringList& deps)
{
    MutexLocker locker(&m_mutex);

    StringMap& map = m_fileHashes[master];
    if (getFileHash(master) != map[master])
        return true;

    StringList::const_iterator it = deps.begin();
    for (; it != deps.end(); ++it) {
        if (getFileHash(*it) != map[*it])
            return true;
    }
    return false;
}

void HashGroups::serializeHashGroups(std::ostream& out)
{
    std::map<std::string, StringMap>::iterator mapMapIt = m_fileHashes.begin();

    for (; mapMapIt != m_fileHashes.end(); ++mapMapIt) {
        if (mapMapIt->second.empty())
            continue;

        out << "hashGroup {\n";
        // Write the key first!
        std::string name(mapMapIt->first);
        stringReplace(name, "\"", "\\\"");
        out << "    master = \"" << name << "\",\n";
        out << "    masterHash = \"" << mapMapIt->second[name] << "\",\n";

        // Write deps files hashes
        out << "    deps = {\n";
        StringMap::const_iterator it = mapMapIt->second.begin();
        for (; it != mapMapIt->second.end(); ++it) {
            // Skip the file hash if it's the key file!
            if (it->first == mapMapIt->first || it->first.empty())
                continue;

            name = it->first;
            stringReplace(name, "\"", "\\\"");
            out << "        {\n";
            out << "            file = \"" << name << "\",\n";

            std::string value(it->second);
            out << "            hash = \"" << value << "\",\n";
            out << "        },\n";
        }
        out << "    }\n";
        out << "}\n\n";
    }
}

void HashGroups::loadHashGroup(lua_State* L)
{
    std::string master = getField<std::string>(L, "master");
    std::string masterHash = getField<std::string>(L, "masterHash");
    if (master.empty())
        return;

    StringMap& map = m_fileHashes[master];
    map[master] = masterHash;
    lua_getfield(L, -1, "deps");

    if (lua_type(L, -1) != LUA_TTABLE) {
        Warn() << "Corrupted hash group for " << master << '.';
        return;
    }

    // loop through deps
    int depsIdx = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, depsIdx) != 0) {
        std::string file = getField<std::string>(L, "file");
        std::string hash = getField<std::string>(L, "hash");
        map[file] = hash;
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
}

void HashGroups::updateHashGroup(const std::string& master, const StringList& deps)
{
    MutexLocker locker(&m_mutex);
    StringMap& map = m_fileHashes[master];
    map[master] = getFileHash(master);

    StringList::const_iterator it = deps.begin();
    for (; it != deps.end(); ++it)
        map[*it] = getFileHash(*it);
}





