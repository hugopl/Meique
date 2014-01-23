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

#include "jobmanager.h"
#include "logger.h"
#include "job.h"
#include <iomanip>
#include "mutexlocker.h"

JobManager::JobManager(unsigned maxJobRunning)
    : m_maxJobsRunning(maxJobRunning)
    , m_jobsRunning(0)
    , m_errorOccured(false)
{
    pthread_mutex_init(&m_jobsRunningMutex, 0);
    pthread_cond_init(&m_needJobsCond, 0);
    pthread_cond_init(&m_allDoneCond, 0);
}

JobManager::~JobManager()
{
}

void JobManager::printReportLine(const Job* job) const
{
    Notice() << job->name();
}

bool JobManager::run()
{
    m_jobCount = 0;
    m_jobsNotIdle = 0;
    m_jobsProcessed = 0;

    while (!m_errorOccured) {
        MutexLocker locker(&m_jobsRunningMutex);
        if (m_jobsRunning >= m_maxJobsRunning) {
            pthread_cond_wait(&m_needJobsCond, &m_jobsRunningMutex);
        }

        if (m_errorOccured)
            break;

        // Now select a valid job
        Job* job = onNeedMoreJobs();
        if (!job)
            break;
        job->onFinished = [&](int result) { onJobFinished(result); };
        job->run();
        m_jobsRunning++;
        m_jobsNotIdle++;
        printReportLine(job);
    }

    MutexLocker locker(&m_jobsRunningMutex);
    if (m_jobsRunning) {
        Notice() << "Waiting for unfinished jobs...";
        pthread_cond_wait(&m_allDoneCond, &m_jobsRunningMutex);
    }

    return !m_errorOccured;
}

void JobManager::onJobFinished(int result)
{
//    MutexLocker locker(&m_jobsRunningMutex);
    m_jobsRunning--;
    m_jobsProcessed++;
    if (result)
        m_errorOccured = true;
    if (m_jobsRunning < m_maxJobsRunning)
        pthread_cond_signal(&m_needJobsCond);
    if (m_jobsRunning == 0)
        pthread_cond_signal(&m_allDoneCond);

}
