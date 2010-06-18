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

#ifndef JOB_H
#define JOB_H
#include "basictypes.h"
#include <pthread.h>

class JobListenner;

class Job
{
public:
    enum Status {
        Idle,
        Scheduled,
        Running,
        Finished
    };

    Job();
    virtual ~Job() {}
    void run();
    void setDescription(const std::string& description) { m_description = description; }
    std::string description() const { return m_description; }
    Status status() const;
    void setDependencies(const std::list<Job*>& jobList) { m_dependencies = jobList; }
    bool hasShowStoppers() const;
    void addJobListenner(JobListenner* listenner);
    int result() const { return m_result; }
protected:
    virtual int doRun() = 0;
private:
    std::string m_description;
    std::list<Job*> m_dependencies;
    Status m_status;
    mutable pthread_mutex_t m_statusMutex;
    pthread_t m_thread;
    std::list<JobListenner*> m_listenners;
    int m_result;

    Job(const Job&);
    Job& operator=(const Job&);

    friend void* initJobThread(void*);
};

#endif // JOB_H
