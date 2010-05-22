#ifndef TARGET_H
#define TARGET_H
#include "basictypes.h"

class Config;
class Compiler;
struct lua_State;
class MeiqueScript;

class Target
{
public:
    Target(const std::string& name, MeiqueScript* script);
    virtual ~Target();
    TargetList dependencies();
    void run(Compiler* compiler);
    lua_State* luaState();
    Config& config();
    const std::string& name() const { return m_name; }
    const std::string& directory();
protected:
    virtual void doRun(Compiler* compiler);
    void getLuaField(const char* field);
private:
    std::string m_name;
    std::string m_directory;
    MeiqueScript* m_script;
    TargetList m_dependencies;
    bool m_dependenciesCached;

    Target(const Target&);
    Target operator=(const Target&);
};

#endif
