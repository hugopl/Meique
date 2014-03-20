/*
    This file is part of the Meique project
    Copyright (C) 2010-2013 Hugo Parente Lima <hugo.pl@gmail.com>

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
    enum ExecOptions {
        None,
        MergeErr
    };

    int exec(const char* cmd, std::string* output = 0, const char* workingDir = 0, ExecOptions options = None);
    inline int exec(const std::string& cmd, std::string* output = 0, const char* workingDir = 0, ExecOptions options = None)
    {
        return exec(cmd.c_str(), output, workingDir, options);
    }

    /// Like cd command.
    void cd(const char* dir);
    inline void cd(const std::string& dir) { cd(dir.c_str()); }
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
    std::string baseName(const std::string& path);

    StringList getOSType();

    int numberOfCPUCores();

    unsigned long getTimeInMillis();
    /// return -x, 0 or +x if file1 is newer, same age or older than file2.
    /// i.e. file2.timestamp - file1.timestamp
    int timestampCompare(const std::string& file1, const std::string& file2);

    /// Path separator used in the current platform, / on unices and \ on MS Windows
    extern const char PathSep;
    /// Returns the canonical form of \p path
    std::string normalizeFilePath(const std::string& path);
    /// Returns the canonical form of \p path + OS::PathSep
    std::string normalizeDirPath(const std::string& path);

    void setCTRLCHandler(void (*func)());

    void install(const std::string& sourceFile, const std::string& destDir);
    void uninstall(const std::string& file);
    std::string defaultInstallPrefix();

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
