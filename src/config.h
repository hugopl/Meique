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
    StringMap arguments() const { return m_args; }
    std::string sourceRoot() const { return m_sourceRoot; }
    bool isInConfigureMode() const { return m_mode == ConfigureMode; }
    bool isInBuildMode() const { return m_mode == BuildMode; }
    void setUserOptions(const StringMap& userOptions);
    void saveCache();
private:
    std::string m_mainArgument;
    std::string m_sourceRoot;
    Mode m_mode;
    StringMap m_args;
    StringMap m_userOptions;

    // disable copy
    Config(const Config&);
    Config& operator=(const Config&);
    void parseArguments(int argc, char** argv);
    void detectMode();
    void loadCache();
    static int readOption(lua_State* L);
    static int readMeiqueConfig(lua_State* L);
};

#endif
