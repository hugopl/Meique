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

#ifndef OS_H
#define OS_H

#include "basictypes.h"

namespace OS
{
    int exec(const std::string& cmd, const StringList& args, std::string* output = 0, const std::string& workingDir = std::string());
    inline int exec(const std::string& cmd, const std::string& arg, std::string* output = 0, const std::string& workingDir = std::string())
    {
        StringList args;
        args.push_back(arg);
        return exec(cmd, args, output, workingDir);
    }
    /// Like cd command.
    void cd(const std::string& dir);
    /// Like pwd command.
    std::string pwd();
    /// Like mkdir -p.
    void mkdir(const std::string& dir);
    /// Returns true if \p fileName exists.
    bool fileExists(const std::string& fileName);
    /// Removes a file from file system, returns true on success.
    bool rm(const std::string& fileName);
    /// Returns the current process id
    unsigned long getPid();
    /// Returns the value of an environment variable.
    std::string getEnv(const std::string& variable);

    std::string dirName(const std::string& path);

    StringList getOSType();

    class ChangeWorkingDirectory
    {
    public:
        ChangeWorkingDirectory(const std::string& dir)
        {
            m_oldDir = pwd();
            cd(dir);
        }
        ~ChangeWorkingDirectory()
        {
            cd(m_oldDir);
        }
    private:
        std::string m_oldDir;
    };
}

#endif
