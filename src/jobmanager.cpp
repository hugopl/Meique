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
#include "jobfactory.h"

#include <cmath>
#include <functional>
#include <iomanip>

JobManager::JobManager(JobFactory& jobFactory, unsigned maxJobRunning)
    : m_jobFactory(jobFactory)
    , m_maxJobsRunning(maxJobRunning)
    , m_jobsRunning(0)
    , m_errorOccured(false)
{
}

JobManager::~JobManager()
{
}

void JobManager::printReportLine(const Job* job) const
{
    Manipulators color;
    switch (job->name().empty() ? 0 : job->name()[0]) {
    case 'C':
        color = Green;
        break;
    case 'L':
        color = Magenta;
        break;
    case 'R':
        color = Blue;
        break;
    }

    // The tree is expanded before any jobs get created, so it's safe to cache this.
    static unsigned nodeCount = m_jobFactory.nodeCount();
    static unsigned digits = nodeCount > 0 ? log10(nodeCount) + 1 : 1;

    Notice() << '[' << std::setw(digits) << m_jobFactory.processedNodes() << '/' << nodeCount << "] " << color << job->name();
}

bool JobManager::run()
{
    while (!m_errorOccured) {
        {
            std::unique_lock<std::mutex> lock(m_jobsRunningMutex);
            if (m_jobsRunning >= m_maxJobsRunning)
                m_needJobsCond.wait(lock);
        }

        if (m_errorOccured)
            break;

        Job* job = m_jobFactory.createJob();
        if (!job)
            break;
        job->onFinished = [&](int result) { onJobFinished(result); };
        job->run();

        std::lock_guard<std::mutex> lock(m_jobsRunningMutex);
        m_jobsRunning++;
        printReportLine(job);
    }

    std::unique_lock<std::mutex> lock(m_jobsRunningMutex);
    if (m_jobsRunning) {
        Notice() << "Waiting for unfinished jobs...";
        m_allDoneCond.wait(lock);
    }

    return !m_errorOccured;
}

void JobManager::onJobFinished(int result)
{
    std::lock_guard<std::mutex> lock(m_jobsRunningMutex);
    m_jobsRunning--;
    if (result)
        m_errorOccured = true;
    if (m_jobsRunning < m_maxJobsRunning)
        m_needJobsCond.notify_all();
    if (m_jobsRunning == 0)
        m_allDoneCond.notify_all();
}
