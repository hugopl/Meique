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

Job::Job(const std::string& command, const StringList& args) : m_command(command), m_args(args)
{
}

void Job::run()
{
    OS::ChangeWorkingDirectory dirChanger(m_workingDir);
    OS::exec(m_command, m_args);
}

std::string Job::workingDirectory()
{
    return m_workingDir;
}
