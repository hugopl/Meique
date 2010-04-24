#include "target.h"
#include "luacpputil.h"
extern "C" {
    #include <lua.h>
}
#include "logger.h"
#include "meiquescript.h"
#include "os.h"

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

    if (m_name != "all") {
        OS::mkdir(directory());
        OS::ChangeWorkingDirectory dirChanger(directory());
        doRun(compiler);
    }
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

const std::string& Target::directory()
{
    if (m_directory.empty()) {
        getLuaField("_dir");
        m_directory = lua_tocpp<std::string>(luaState(), -1);
        lua_pop(luaState(), 1);
        if (!m_directory.empty())
            m_directory += '/';
    }
    return m_directory;
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

