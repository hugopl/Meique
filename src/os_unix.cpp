#include "os.h"
#include <unistd.h>
#include <sys/wait.h>
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

    return system(cmdline.c_str());
}

}
