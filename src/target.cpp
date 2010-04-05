#include "target.h"
extern "C" {
    #include <lua.h>
}
#include "logger.h"
#include "meiquescript.h"

Target::Target(const std::string& name, MeiqueScript* script)
              : m_name(name), m_script(script), m_dependenciesCached(false)
{
}

Target::~Target()
{
}

TargetList Target::dependencies()
{
    if (!m_dependenciesCached) {
        Notice() << "Scanning dependencies of target \"" << m_name << '"';
        if (m_name == "all") {
            TargetList targets = m_script->targets();
            targets.remove(this);
            return targets;
        } else {
        }
    }
    return m_dependencies;
}

void Target::run(Compiler* compiler)
{
    Notice() << "Running target \"" << m_name << '"';
    // TODO: Support parallel jobs
    TargetList deps = dependencies();
    TargetList::iterator it = deps.begin();
    for (; it != deps.end(); ++it)
        (*it)->run(compiler);

    doRun(compiler);
}

void Target::doRun(Compiler*)
{
}

lua_State* Target::luaState()
{
    return m_script->luaState();
}

const Config& Target::config() const
{
    return m_script->config();
}

void Target::getLuaField(const char* field)
{
    lua_pushlightuserdata(luaState(), (void *)this);
    lua_gettable(luaState(), LUA_REGISTRYINDEX);
    lua_getfield(luaState(), -1, field);
    // remove table from stack
    lua_insert(luaState(), -2);
    lua_pop(luaState(), 1);
}

