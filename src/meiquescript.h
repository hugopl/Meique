#ifndef MEIQUESCRIPT_H
#define MEIQUESCRIPT_H

#include <string>
#include "basictypes.h"
#include "luastate.h"

class Config;

struct MeiqueOption
{
    MeiqueOption() {}
    MeiqueOption(const std::string& descr, const std::string& defVal) : description(descr), defaultValue(defVal) {}
    std::string description;
    std::string defaultValue;
};

typedef std::map<std::string, MeiqueOption> OptionsMap;

class MeiqueScript
{
public:
    MeiqueScript(const Config& config);
    void exec();
    OptionsMap options();
private:
    LuaState m_L;
    std::string m_scriptName;
    OptionsMap m_options;

    // disable copy
    MeiqueScript(const MeiqueScript&);
    MeiqueScript& operator=(const MeiqueScript&);

    void translateLuaError(int code);
    void exportApi();
};

#endif
