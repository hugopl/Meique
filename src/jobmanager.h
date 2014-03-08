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

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <mutex>
#include <condition_variable>

class Job;
class JobFactory;
class JobQueue;

class JobManager
{
public:
    JobManager(JobFactory& jobFactory, unsigned maxJobRunning);
    ~JobManager();

    bool run();
private:
    JobFactory& m_jobFactory;

    unsigned m_maxJobsRunning;
    unsigned m_jobsRunning;
    std::mutex m_jobsRunningMutex;

    bool m_errorOccured;
    std::condition_variable m_needJobsCond;
    std::condition_variable m_allDoneCond;

    void printReportLine(const Job*) const;
    void onJobFinished(int result);

    JobManager(const JobManager&) = delete;
};

#endif // JOBMANAGER_H
