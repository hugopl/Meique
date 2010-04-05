#ifndef OS_H
#define OS_H

#include "basictypes.h"

namespace OS
{
    int exec(const std::string& cmd, const StringList& args);
}

#endif
