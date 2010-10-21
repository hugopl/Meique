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
    m_nextJob = m_jobs.end();
}

JobQueue::~JobQueue()
{
    deleteAll(m_jobs);
}

void JobQueue::addJob(Job* job)
{
    bool resetIterator = m_jobs.empty();
    m_jobs.push_back(job);
    if (resetIterator)
        m_nextJob = m_jobs.begin();
}

Job* JobQueue::getNextJob()
{
    if (m_nextJob == m_jobs.end())
        return 0;
    Job* job = *m_nextJob;
    if (job->hasShowStoppers())
        return 0;

    ++m_nextJob;
    return job;
}

bool JobQueue::hasShowStoppers() const
{
    std::list<JobQueue*>::const_iterator it = m_dependencies.begin();
    for (; it != m_dependencies.end(); ++it)
        if ((*it)->hasShowStoppers())
            return true;
    return false;
}

void JobQueue::addDependency(JobQueue* queue)
{
    m_dependencies.push_back(queue);
}

bool JobQueue::isEmpty() const
{
    if (m_jobs.empty())
        return true;
    Job::Status lastJobStatus = m_jobs.back()->status();
    return lastJobStatus == Job::FinishedWithSuccess || lastJobStatus == Job::FinishedButFailed;
}
