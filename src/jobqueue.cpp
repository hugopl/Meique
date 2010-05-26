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

#include "jobqueue.h"
#include "job.h"

JobQueue::JobQueue()
{
}

JobQueue::~JobQueue()
{
    std::list<Job*>::iterator it = m_jobs.begin();
    for (; it != m_jobs.end(); ++it)
        delete *it;
}

void JobQueue::addJob(Job* job)
{
    m_jobs.push_back(job);
}

Job* JobQueue::takeJob()
{
    Job* job = m_jobs.front();
    m_jobs.pop_front();
    return job;
}

