#include "config.h"
#include "logger.h"
#include "luastate.h"
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
    int res = luaL_loadfile(L, MEIQUECACHE);
    if (res)
        Error() << "Error loading " MEIQUECACHE ", this *should* never happen. A bug? maybe...";
    lua_register(L, "option", &readOption);

}

void Config::saveCache()
{
    std::ofstream file(MEIQUECACHE);
    if (!file.is_open())
        Error() << "Can't open " MEIQUECACHE " for write.";
    StringMap::const_iterator it = m_args.begin();
    for (; it != m_args.end(); ++it) {
        std::string name(it->first);
        stringReplace(name, "\"", "\\\"");
        file << "option { \n";
        file << "\tname = \"" << name << "\",\n";
        file << "\tmd5 = \"" << it->second << "\"\n";
        file << "}\n\n";
    }
}

int Config::readOption(lua_State* L)
{

}
