#include "os.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "logger.h"
#include <stdlib.h>

namespace OS
{

int exec(const std::string& cmd, const StringList& args)
{
    std::string cmdline = cmd;
    StringList::const_iterator it = args.begin();
    for (; it != args.end(); ++it) {
        cmdline += ' ';
        cmdline += *it;
    }

    Notice() << cmdline;
    return system(cmdline.c_str());
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
        Error() << "Can't create an empty directory!";
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

}
