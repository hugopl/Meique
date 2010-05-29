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

#include "os.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "logger.h"
#include <stdlib.h>
#include <cstdio>

namespace OS
{

int exec(const std::string& cmd, const StringList& args, std::string* output, const std::string& workingDir)
{
    std::string cmdline = cmd;
    StringList::const_iterator it = args.begin();
    for (; it != args.end(); ++it) {
        cmdline += ' ';
        cmdline += *it;
    }

    Debug() << cmdline;
    int status;
    int out2me[2];  // pipe from external program stdout to meique
//     int err2me[2];  // pipe from external program stderr to meique
    if (output) {
        if (pipe(out2me)/* || pipe(err2me)*/)
            Error() << "Unable to create unix pipes!";
    }

    pid_t pid = fork();
    if (pid == -1) {
        Error() << "Error running command: " << cmdline;
    } else if (!pid) {
        if (!workingDir.empty())
            OS::cd(workingDir);
        if (output) {
            close(out2me[0]);
//             close(err2me[0]);
            dup2(out2me[1], 1);
//             dup2(err2me[1], 2);
        }
        execl("/bin/sh", "sh", "-c", cmdline.c_str(), (char*)0);
        Error() << "Fatal error: shell not found!";
    }

    if (output) {
        close(out2me[1]);
//         close(err2me[1]);
        char buffer[512];
        int bytes;
        while((bytes = read(out2me[0], buffer, sizeof(buffer))) > 0) {
            buffer[bytes] = 0;
            *output += buffer;
        }
    }
    waitpid(pid, &status, 0);
    return status;
}

void cd(const std::string& dir)
{
    if (::chdir(dir.c_str()) == -1)
        Error() << "Error changing to directory " << dir << '.';
}

std::string pwd()
{
    char buffer[512];
    if (!getcwd(buffer, sizeof(buffer)))
        Error() << "Internal error getting the current working directory.";
    std::string result(buffer);
    result += '/';
    return result;
}

static void meiqueMkdir(const std::string& dir)
{
    const char ERROR_MSG[] = "Internal error creating directory ";
    if (::mkdir(dir.c_str(), 0755) == -1 && errno != EEXIST) {
        size_t pos = dir.find_last_of('/');
        if (pos == std::string::npos)
            Error() << ERROR_MSG << dir << '.';
        meiqueMkdir(dir.substr(0, pos));
        if (::mkdir(dir.c_str(), 0755) == -1)
            Error() << ERROR_MSG << dir << '.';
    }
}

void mkdir(const std::string& dir)
{
    if (dir.empty())
        return;
    else if (*(--dir.end()) == '/')
        meiqueMkdir(dir.substr(0, dir.size() - 1));
    else
        meiqueMkdir(dir);
}

bool fileExists(const std::string& fileName)
{
    struct stat fileAtt;
    if (::stat(fileName.c_str(), &fileAtt) != 0)
        return false;
    return S_ISREG(fileAtt.st_mode);
}

bool rm(const std::string& fileName)
{
    Debug() << "rm " << fileName;
    return !::unlink(fileName.c_str());
}

unsigned long getPid()
{
    return ::getpid();
}

std::string getEnv(const std::string& variable)
{
    char* value = ::getenv(variable.c_str());
    return value ? std::string(value) : std::string();
}

}
