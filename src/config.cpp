#include "config.h"
#include "logger.h"
#include "luastate.h"
#include "luacpputil.h"
#include "cppstringssux.h"
#include <fstream>

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
};

Config::Config(int argc, char** argv)
{
    detectMode();
    parseArguments(argc, argv);
}

void Config::parseArguments(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        // non-option, must be the source directory
        if (arg.find("--") != 0) {
            if (m_mainArgument.size()) {
                // TODO: A better error message
                Error() << "The main argument already was specified [main argument: " << m_mainArgument << "].";
            } else {
                m_mainArgument = arg;
            }
        } else {
            arg.erase(0, 2);
            size_t equalPos = arg.find("=");
            if (equalPos == std::string::npos)
                m_args.insert(std::make_pair(arg, std::string()));
            else
                m_args.insert(std::make_pair(arg.substr(0, equalPos), arg.substr(equalPos+1, arg.size()-equalPos)));
        }
    }

    if (m_mode == ConfigureMode)
        m_sourceRoot = m_mainArgument;
}

void Config::detectMode()
{
    std::ifstream file(MEIQUECACHE);
    if (file) {
        file.close();
        m_mode = BuildMode;
        loadCache();
    } else {
        m_mode = ConfigureMode;
    }
}

void Config::loadCache()
{
    LuaState L;
    lua_register(L, "userOption", &readOption);
    lua_register(L, "meiqueConfig", &readMeiqueConfig);
    // put a pointer to this instance of Config in lua registry, the key is the L address.
    lua_pushlightuserdata(L, (void *)L);
    lua_pushlightuserdata(L, (void *)this);
    lua_settable(L, LUA_REGISTRYINDEX);

    int res = luaL_loadfile(L, MEIQUECACHE);
    if (res)
        Error() << "Error loading " MEIQUECACHE ", this *should* never happen. A bug? maybe...";
    if (lua_pcall(L, 0, 0, 0))
        Error() << "Error loading " MEIQUECACHE ".";
}

// Retrieve the Config instance
static Config* getSelf(lua_State* L)
{
    lua_pushlightuserdata(L, (void *)L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Config* self = reinterpret_cast<Config*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return self;
}

void Config::saveCache()
{
    std::ofstream file(MEIQUECACHE);
    if (!file.is_open())
        Error() << "Can't open " MEIQUECACHE " for write.";
    StringMap::const_iterator it = m_userOptions.begin();
    for (; it != m_userOptions.end(); ++it) {
        std::string name(it->first);
        stringReplace(name, "\"", "\\\"");
        std::string value(it->second);
        stringReplace(name, "\"", "\\\"");
        file << "userOption {\n"
                "    name = \"" << name << "\",\n"
                "    value = \"" << value << "\"\n"
                "}\n\n";
    }

    std::string sourceRoot(m_sourceRoot);
    stringReplace(sourceRoot, "\"", "\\\"");
    file << "meiqueConfig {\n"
            "    sourceRoot = \"" << sourceRoot << "\"\n"
            "}\n\n";

}

int Config::readOption(lua_State* L)
{
    Config* self = getSelf(L);
    std::string name = GetField<std::string>(L, "name");
    std::string value = GetField<std::string>(L, "value");
    self->m_userOptions[name] = value;
    return 0;
}

int Config::readMeiqueConfig(lua_State* L)
{
    Config* self = getSelf(L);
    self->m_sourceRoot = GetField<std::string>(L, "sourceRoot");
    return 0;
}

void Config::setUserOptions(const StringMap& userOptions)
{
    m_userOptions = userOptions;
}
