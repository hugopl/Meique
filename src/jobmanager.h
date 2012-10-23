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

#ifndef JOBMANAGER_H
#define JOBMANAGER_H
#include <list>
#include <pthread.h>
#include <map>
#include "joblistenner.h"

class Job;
class JobQueue;

class JobManager : public JobListenner
{
public:
    JobManager();
    ~JobManager();
    void setJobCountLimit(int n) { m_maxJobsRunning = n; }
    void addJobQueue(JobQueue* queue);
    void processJobs();

    void jobFinished(Job* job);
private:
    std::list<JobQueue*> m_queues;
    int m_maxJobsRunning;
    int m_jobsRunning;
    int m_jobsProcessed;
    int m_jobCount;
    int m_jobsNotIdle;
    bool m_errorOccured;
    pthread_mutex_t m_jobsRunningMutex;
    pthread_cond_t m_needJobsCond;
    pthread_cond_t m_allDoneCond;

    void printReportLine(const Job*) const;

    JobManager(const JobManager&);
    JobManager& operator=(const JobManager&);
};

#endif // JOBMANAGER_H
