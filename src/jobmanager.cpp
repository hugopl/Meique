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

JobManager::JobManager() : m_maxJobsRunning(1), m_jobsRunning(0)
{
    pthread_mutex_init(&m_jobsRunningMutex, 0);
    pthread_cond_init(&m_jobsRunningCond, 0);
}

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

void JobManager::processJobs()
{
    m_jobCount = 0;
    std::list<JobQueue*>::const_iterator it = m_queues.begin();
    for (; it != m_queues.end(); ++it)
        m_jobCount += (*it)->jobCount();

    m_jobsProcessed = 0;
    while (m_jobsProcessed < m_jobCount) {
        // Select a valid queue
        std::list<JobQueue*>::iterator queueIt = m_queues.begin();
        JobQueue* queue = *queueIt;
        while (queue->hasShowStoppers() || queue->isEmpty()) {
            queue = *(++queueIt);
            if (queueIt == m_queues.end())
                return;
        }

        pthread_mutex_lock(&m_jobsRunningMutex);
        if (m_jobsRunning >= m_maxJobsRunning)
            pthread_cond_wait(&m_jobsRunningCond, &m_jobsRunningMutex);
        // Now select a valid job
        if (Job* job = queue->getNextJob()) {
            job->addJobListenner(this);
            job->run();
            m_jobsRunning++;
            Notice() << job->description();
        }
        pthread_mutex_unlock(&m_jobsRunningMutex);
    }
}

void JobManager::jobFinished(Job* job)
{
    pthread_mutex_lock(&m_jobsRunningMutex);
    m_jobsRunning--;
    m_jobsProcessed++;
    if (m_jobsRunning < m_maxJobsRunning)
        pthread_cond_signal(&m_jobsRunningCond);
    pthread_mutex_unlock(&m_jobsRunningMutex);
}
