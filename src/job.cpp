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
#include "os.h"
#include "jobqueue.h"
#include "joblistenner.h"

Job::Job(const std::string& command, const StringList& args) : m_command(command), m_args(args), m_status(Idle), m_result(0)
{
    pthread_mutex_init(&m_statusMutex, 0);
}

void* initJobThread(void* ptr)
{
    Job* job = reinterpret_cast<Job*>(ptr);
    job->doRun();
    return 0;
}

void Job::run()
{
    pthread_mutex_lock(&m_statusMutex);
    m_status = Scheduled;
    pthread_mutex_unlock(&m_statusMutex);
    pthread_create(&m_thread, 0, initJobThread, this);
}

void Job::doRun()
{
    pthread_mutex_lock(&m_statusMutex);
    m_status = Running;
    pthread_mutex_unlock(&m_statusMutex);

    m_result = OS::exec(m_command, m_args, 0, m_workingDir);

    pthread_mutex_lock(&m_statusMutex);
    m_status = Finished;
    pthread_mutex_unlock(&m_statusMutex);

    std::list<JobListenner*>::iterator it = m_listenners.begin();
    for (; it != m_listenners.end(); ++it)
        (*it)->jobFinished(this);
}

std::string Job::workingDirectory()
{
    return m_workingDir;
}

Job::Status Job::status() const
{
    pthread_mutex_lock(&m_statusMutex);
    Status s = m_status;
    pthread_mutex_unlock(&m_statusMutex);
    return s;
}

bool Job::hasShowStoppers() const
{
    std::list<Job*>::const_iterator it = m_dependencies.begin();
    for (; it != m_dependencies.end(); ++it) {
        if ((*it)->status() != Finished || (*it)->hasShowStoppers())
            return true;
    }
    return false;
}

void Job::addJobListenner(JobListenner* listenner)
{
    m_listenners.push_back(listenner);
}
