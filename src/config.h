#ifndef CONFIG_H
#define CONFIG_H

#include "basictypes.h"

#define MEIQUECACHE "meiquecache.lua"

struct lua_State;

class Config
{
public:
    enum Mode {
        ConfigureMode,
        BuildMode
    };

    Config(int argc, char** argv);
    Mode mode() const { return m_mode; }

    std::string mainArgument() const { return m_mainArgument; }
    std::string sourceRoot() const { return m_sourceRoot; }
private:
    std::string m_mainArgument;
    std::string m_sourceRoot;
    Mode m_mode;
    StringMap m_args;

    // disable copy
    Config(const Config&);
    Config& operator=(const Config&);
    void parseArguments(int argc, char** argv);
    void detectMode();
    void loadCache();
    void saveCache();
    static int readOption(lua_State* L);
};

#endif
