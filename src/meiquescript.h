#ifndef MEIQUESCRIPT_H
#define MEIQUESCRIPT_H

#include <string>
#include "basictypes.h"
#include "luastate.h"

class Config;

class MeiqueScript
{
public:
    MeiqueScript(const Config& config);
    void exec();
private:
    LuaState m_L;
    std::string m_scriptName;

    // disable copy
    MeiqueScript(const MeiqueScript&);
    MeiqueScript& operator=(const MeiqueScript&);

    void translateLuaError(int code);
    void exportApi();
};

#endif
