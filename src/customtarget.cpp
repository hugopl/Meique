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
#include "jobqueue.h"
#include "lua.h"
#include "luajob.h"
#include "config.h"
#include "logger.h"

JobQueue* CustomTarget::doRun(Compiler* compiler)
{
    Config& cfg = config();
    bool isOutdated = false;
    std::string sourceDir = config().sourceRoot() + directory();
    StringList files = this->files();

    // Check if we need to run this target.
    if (!files.empty()) {
        StringList::iterator it = files.begin();
        for (; it != files.end(); ++it) {
            if (it->empty())
                continue;
            *it = it->at(0) == '/' ? *it : sourceDir + *it;
            if (cfg.isHashGroupOutdated(*it)) {
                isOutdated = true;
                break;
            }
        }
    } else {
        isOutdated = true;
    }

    JobQueue* queue = new JobQueue;
    if (isOutdated) {
        // Put the lua function on stack
        getLuaField("_func");
        LuaJob* job = new LuaJob(luaState(), 0);
        job->setDescription("Running custom target function for " + name());
        job->addJobListenner(this);
        m_job2Sources[job] = files;
        queue->addJob(job);
    }
    return queue;
}

void CustomTarget::jobFinished(Job* job)
{
    if (!job->result()) {
        StringList& files = m_job2Sources[job];
        for (StringList::const_iterator it = files.begin(); it != files.end(); ++it)
            config().updateHashGroup(*it);
    }
    m_job2Sources.erase(job);
}
