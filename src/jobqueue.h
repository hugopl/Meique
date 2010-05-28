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

#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <list>

class Job;

class JobQueue
{
public:
    JobQueue();
    ~JobQueue();
    void addJob(Job* job);
    Job* getNextJob();
    bool isEmpty() const;
    int jobCount() const { return m_jobs.size(); }
    std::list<Job*> idleJobs() const { return m_jobs; }
    bool hasShowStoppers() const;
private:
    std::list<Job*> m_jobs;
    std::list<Job*>::iterator m_nextJob;
    std::list<JobQueue*> m_dependencies;

    JobQueue(const JobQueue&);
    JobQueue& operator=(const JobQueue&);
};

#endif // JOBQUEUE_H
