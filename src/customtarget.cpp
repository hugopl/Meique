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

#include "customtarget.h"
#include "lua.h"
#include "luajob.h"
#include "meiquecache.h"
#include "logger.h"
#include "meiquescript.h"
#include "luacpputil.h"

#if 0
JobQueue* CustomTarget::doRun(Compiler* compiler)
{
    MeiqueCache* mcache = cache();
    std::string sourceDir = script()->sourceDir() + directory();
    std::string buildDir = script()->buildDir() + directory();

    lua_State* L = luaState();
    getLuaField("_outputs");
    StringList outputs;
    readLuaList(L, lua_gettop(L), outputs);
    lua_pop(L, 1);

    bool outputDoesntExist = false;
    bool isOutdated = false;
    for (StringList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        if (!OS::fileExists(buildDir + *it)) {
            outputDoesntExist = true;
            isOutdated = true;
            break;
        }
    }

    StringList files = this->files();
    StringList outdatedFiles;

    // Check if we need to run this target.
    if (!files.empty()) {
        StringList::iterator it = files.begin();
        for (; it != files.end(); ++it) {
            if (it->empty())
                continue;
            *it = it->at(0) == '/' ? *it : sourceDir + *it;
            if (outputDoesntExist || mcache->isHashGroupOutdated(*it)) {
                isOutdated = true;
                outdatedFiles.push_back(*it);
            }
        }
    } else if (outputs.empty()) {
        isOutdated = true; // custom targets without input and output always run
    }

    JobQueue* queue = new JobQueue;
    if (isOutdated) {
        // Put the lua function on stack
        getLuaField("_func");
        createLuaArray(luaState(), outdatedFiles);
        LuaJob* job = new LuaJob(luaState(), 1);
        job->setName(name());
        job->setType(Job::CustomTarget);
        job->addJobListenner(this);
        m_job2Sources[job] = files;
        queue->addJob(job);
    }
    return queue;
}
#endif

void CustomTarget::jobFinished(Job* job)
{
    if (!job->result()) {
        StringList& files = m_job2Sources[job];
        for (StringList::const_iterator it = files.begin(); it != files.end(); ++it)
            cache()->updateHashGroup(*it);
    }
    m_job2Sources.erase(job);
}
