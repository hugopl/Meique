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

#include "job.h"
#include "joblistenner.h"
#include "mutexlocker.h"

Job::Job() : m_status(Idle), m_type(Compilation), m_result(0)
{
    pthread_mutex_init(&m_statusMutex, 0);
}

void* initJobThread(void* ptr)
{
    Job* job = reinterpret_cast<Job*>(ptr);

    pthread_mutex_lock(&job->m_statusMutex);
    job->m_status = Job::Running;
    pthread_mutex_unlock(&job->m_statusMutex);

    job->m_result = job->doRun();

    pthread_mutex_lock(&job->m_statusMutex);
    job->m_status = job->m_result ? Job::FinishedButFailed : Job::FinishedWithSuccess;
    pthread_mutex_unlock(&job->m_statusMutex);

    std::list<JobListenner*>::iterator it = job->m_listenners.begin();
    for (; it != job->m_listenners.end(); ++it)
        (*it)->jobFinished(job);

    return 0;
}

void Job::run()
{
    pthread_mutex_lock(&m_statusMutex);
    m_status = Scheduled;
    pthread_mutex_unlock(&m_statusMutex);
    pthread_create(&m_thread, 0, initJobThread, this);
}

Job::Status Job::status() const
{
    return m_status;
}

bool Job::hasShowStoppers() const
{
    std::list<Job*>::const_iterator it = m_dependencies.begin();
    for (; it != m_dependencies.end(); ++it) {
        if ((*it)->status() != FinishedWithSuccess || (*it)->hasShowStoppers())
            return true;
    }
    return false;
}

void Job::addJobListenner(JobListenner* listenner)
{
    m_listenners.push_back(listenner);
}
