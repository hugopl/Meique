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

#include "target.h"
#include "logger.h"
#include "os.h"
#include "jobqueue.h"

Target::Target(const std::string& name) : m_name(name)
{
}

Target::~Target()
{
}

JobQueue* Target::run(Compiler* compiler)
{
    Notice() << "Running target \"" << m_name << '"';
    OS::mkdir(directory());
    OS::ChangeWorkingDirectory dirChanger(directory());
    return doRun(compiler);
}

JobQueue* Target::doRun(Compiler*)
{
    return new JobQueue;
}
