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

#include "jobmanager.h"
#include "logger.h"
#include "jobqueue.h"
#include "job.h"

JobManager::~JobManager()
{
    std::list<JobQueue*>::iterator it = m_queues.begin();
    for (; it != m_queues.end(); ++it)
        delete *it;
}

void JobManager::addJobQueue(JobQueue* queue)
{
    m_queues.push_back(queue);
}

void JobManager::processJobs(int n)
{
    Debug() << "process jobs, n: " << n;
    // For now, just run the jobs...
    while (!m_queues.empty()) {
        Debug() << "queues size: " << m_queues.size();
        JobQueue* queue = m_queues.front();
        while (!queue->isEmpty()) {
            Job* job = queue->takeJob();
            job->run();
            delete job;
        }
        delete queue;
        m_queues.pop_front();
    }
}

