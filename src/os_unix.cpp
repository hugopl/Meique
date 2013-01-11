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
extern "C" {
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
}
#include "logger.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <libgen.h>
#include "stdstringsux.h"

namespace OS
{

int exec(const std::string& cmd, const StringList& args, std::string* output, const std::string& workingDir)
{
    enum { READ, WRITE };

    std::string cmdline(cmd);
    if (args.size())
        cmdline += ' ' + join(args, " ");

    Debug() << cmdline;
    int status;
    int out2me[2];  // pipe from external program stdout to meique
    if (output && pipe(out2me))
        Error() << "Unable to create unix pipes!";

    pid_t pid = fork();
    if (pid == -1) {
        Error() << "Error forking process to run: " << cmdline;
    } else if (!pid) {
        if (!workingDir.empty())
            OS::cd(workingDir);
        if (output) {
            close(out2me[READ]);
            dup2(out2me[WRITE], 1);
            dup2(out2me[WRITE], 2);
        }
        execl("/usr/bin/env", "-i", "sh", "-c", cmdline.c_str(), (char*)0);
        Error() << "Fatal error: shell not found!";
    }

    if (output) {
        close(out2me[WRITE]);
        char buffer[512];
        int bytes;
        while((bytes = read(out2me[READ], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes] = 0;
            *output += buffer;
        }
        trim(*output);
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

    if (dir.empty())
        return;
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

std::string dirName(const std::string& path)
{
    size_t idx = path.find_last_of('/');
    return idx == std::string::npos ? "/" : path.substr(0, idx + 1);
}

std::string baseName(const std::string& path)
{
    char* str = strdup(path.c_str());
    std::string result(basename(str));
    free(str);
    return result;
}

StringList getOSType()
{
    StringList retVal;
    retVal.push_back("UNIX");
    std::string osName;
    OS::exec("uname", "-s", &osName);
    bool isLinux = osName.find("Linux") != std::string::npos;

    if (isLinux)
        retVal.push_back("LINUX");

    return retVal;
}

unsigned long getTimeInMillis()
{
    timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec * 1000 + t.tv_usec/1000;
}

const char PathSep = '/';

static std::string normalizePath(const std::string& path)
{
    std::string rpath;
    if (path.length() && path[0] != PathSep)
        rpath = OS::pwd() + path;
    else
        rpath = path;
    StringList pathParts = split(rpath, PathSep);

    StringList::iterator it = pathParts.begin();
    for (; it != pathParts.end(); ++it) {
        const std::string& part = *it;
        if (part == ".") {
            StringList::iterator start(it);
            pathParts.erase(start, ++it);
        } else if (part == "..") {
            if  (it == pathParts.begin())
                Error() << "Invalid path given: " << path;

            StringList::iterator start(it);
            pathParts.erase(--start, ++it);
        }
    }

    return PathSep + join(pathParts, "/");
}

std::string normalizeFilePath(const std::string& path)
{
    return normalizePath(path);
}

std::string normalizeDirPath(const std::string& path)
{
    return normalizePath(path) + PathSep;
}

void (*ctrlCHandler)();

extern "C" {
static void _handleCtrlC(int)
{
    ctrlCHandler();
}
}

void setCTRLCHandler(void (*func)())
{
    ctrlCHandler = func;
    signal(SIGINT, _handleCtrlC);
}

void install(const std::string& sourceFile, const std::string& destDir)
{
    // cosmetic stuff :-D
    size_t idx = sourceFile.find_last_of('/');
    std::string sourceFileNoPath = idx == std::string::npos ? sourceFile : sourceFile.substr(idx + 1);
    Notice() << "-- Install " << destDir << sourceFileNoPath;

    mkdir(destDir);
    // FIXME Use the install command or copy files by hand!?
    StringList args;
    args.push_back("-C");
    args.push_back("-D");
    args.push_back(sourceFile);
    args.push_back(destDir);
    int status = OS::exec("install", args);
    if (status)
        Error() << "Error installing " << sourceFile << " into " << destDir;
}

std::string defaultInstallPrefix()
{
    return "/usr/";
}

int timestampCompare(const std::string& file1, const std::string& file2)
{
    struct stat file1Stat;
    struct stat file2Stat;
    int error1 = ::stat(file1.c_str(), &file1Stat) != 0;
    int error2 = ::stat(file2.c_str(), &file2Stat) != 0;

    if (error1 || error2)
        return error1 - error2;

    return file2Stat.st_mtime - file1Stat.st_mtime;
}
}
