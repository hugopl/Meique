/*
    This file is part of the Meique project
    Copyright (C) 2009-2014 Hugo Parente Lima <hugo.pl@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "meiquescript.h"

#include <string>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "meiquescript.h"
#include "cmdline.h"
#include "meiquecache.h"
#include "logger.h"
#include "luacpputil.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"
#include "target.h"
#include "compilabletarget.h"
#include "librarytarget.h"
#include "customtarget.h"
#include "os.h"
#include "stdstringsux.h"
#include "executabletarget.h"
#include "meiqueregex.h"
#include "meiqueversion.h"

enum TargetTypes {
    EXECUTABLE_TARGET = 1,
    LIBRARY_TARGET,
    CUSTOM_TARGET
};

// Key used to store the meique script object on lua registry
#define MEIQUESCRIPTOBJ_KEY "MeiqueScript"
// Key used to store the meique options, see option function
#define MEIQUEOPTIONS_KEY "MeiqueOptions"

static int requiresMeique(lua_State* L);
static int findPackage(lua_State* L);
static int copyFile(lua_State* L);
static int configureFile(lua_State* L);
static int option(lua_State* L);
static int meiqueAutomoc(lua_State* L);
static int meiqueQtResource(lua_State* L);

extern const char meiqueApi[];

static MeiqueScript* getMeiqueScriptObject(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, MEIQUESCRIPTOBJ_KEY);
    MeiqueScript* obj = reinterpret_cast<MeiqueScript*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return obj;
}

static int meiqueSourceDir(lua_State* L)
{
    MeiqueScript* script = getMeiqueScriptObject(L);
    lua_pushstring(L, script->sourceDir().c_str());
    return 1;
}

static int meiqueBuildDir(lua_State* L)
{
    MeiqueScript* script = getMeiqueScriptObject(L);
    lua_pushstring(L, script->buildDir().c_str());
    return 1;
}

static int isOutDated(lua_State* L)
{
    std::string master = lua_tocpp<std::string>(L, 1);
    if (master.empty())
        luaError(L, "isOutDated(filePath) called with wrong arguments.");

    MeiqueScript* script = getMeiqueScriptObject(L);
    bool res = script->cache()->isHashGroupOutdated(master);
    lua_pushboolean(L, res);
    return 1;
}

static int setUpToDate(lua_State* L)
{
    std::string master = lua_tocpp<std::string>(L, 1);
    if (master.empty())
        luaError(L, "setUpToDate(filePath) called with wrong arguments.");

    MeiqueScript* script = getMeiqueScriptObject(L);
    script->cache()->updateHashGroup(master);
    return 0;
}

MeiqueScript::MeiqueScript() : m_cache(new MeiqueCache), m_cmdLine(0)
{
    m_cache->loadCache();
    m_scriptName = m_cache->sourceDir() + "meique.lua";
}

MeiqueScript::MeiqueScript(const std::string scriptName, const CmdLine* cmdLine) : m_cache(new MeiqueCache), m_cmdLine(cmdLine)
{
    m_cache->setBuildType(cmdLine->boolArg("debug") ? MeiqueCache::Debug : MeiqueCache::Release);
    m_cache->setInstallPrefix(cmdLine->arg("install-prefix"));

    m_scriptName = OS::normalizeFilePath(scriptName);
}

MeiqueScript::~MeiqueScript()
{
    TargetsMap::const_iterator it = m_targets.begin();
    for (; it != m_targets.end(); ++it)
        delete it->second;
    delete m_cache;
}

void MeiqueScript::setSourceDir(const std::string& sourceDir)
{
    m_cache->setSourceDir(sourceDir);
}

void MeiqueScript::setBuildDir(const std::string& buildDir)
{
    m_buildDir = OS::normalizeDirPath(buildDir);
}

std::string MeiqueScript::sourceDir() const
{
    return m_cache->sourceDir();
}

void MeiqueScript::exec()
{
    OS::ChangeWorkingDirectory dirChanger(sourceDir());

    exportApi();
    translateLuaError(m_L, luaL_loadfile(m_L, m_scriptName.c_str()), m_scriptName);

    luaPCall(m_L, 0, 0, m_scriptName);
    extractTargets();
}

void MeiqueScript::exportApi()
{
    // Opens all standard Lua libraries
    luaL_openlibs(m_L);

    // export lua API
    int sanityCheck = luaL_loadstring(m_L, meiqueApi);
    translateLuaError(m_L, sanityCheck, "[meiqueApi]");
    sanityCheck = lua_pcall(m_L, 0, 0, 0);
    translateLuaError(m_L, sanityCheck, m_scriptName);

    enableBuitinScopes();

    lua_register(m_L, "requiresMeique", &requiresMeique);
    lua_register(m_L, "findPackage", &findPackage);
    lua_register(m_L, "configureFile", &configureFile);
    lua_register(m_L, "copyFile", &copyFile);
    lua_register(m_L, "option", &option);
    lua_register(m_L, "meiqueSourceDir", &meiqueSourceDir);
    lua_register(m_L, "meiqueBuildDir", &meiqueBuildDir);
    lua_register(m_L, "isOutdated", &isOutDated);
    lua_register(m_L, "setUpToDate", &setUpToDate);
    lua_register(m_L, "_meiqueAutomoc", &meiqueAutomoc);
    lua_register(m_L, "_meiqueQtResource", &meiqueQtResource);
    lua_settop(m_L, 0);

    // Add options table to lua registry
    lua_newtable(m_L);
    lua_setfield(m_L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);

    // Export MeiqueScript class to lua registry
    lua_pushlightuserdata(m_L, (void*) this);
    lua_setfield(m_L, LUA_REGISTRYINDEX, MEIQUESCRIPTOBJ_KEY);
}

template<>
MeiqueOption lua_tocpp<MeiqueOption>(lua_State* L, int index)
{
    if (!lua_istable(L, index))
        throw Error("Expecting a lua table! Got " + std::string(lua_typename(L, lua_type(L, index))));
    IntStrMap map;
    readLuaTable(L, index, map);
    lua_pop(L, 1);
    return MeiqueOption(map[1], map[2]);
}

OptionsMap MeiqueScript::options()
{
    if (m_options.size())
        return m_options;

    lua_getfield(m_L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);
    int tableIndex = lua_gettop(m_L);
    readLuaTable(m_L, tableIndex, m_options);
    lua_pop(m_L, 1);
    return m_options;
}

void MeiqueScript::extractTargets()
{
    lua_getglobal(m_L, "_meiqueAllTargets");
    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    while (lua_next(m_L, tableIndex) != 0) {
        // Get target type
        lua_getfield(m_L, -1, "_type");
        int targetType = lua_tocpp<int>(m_L, -1);
        lua_pop(m_L, 1);

        std::string targetName = lua_tocpp<std::string>(m_L, -2);

        Target* target = 0;
        switch (targetType) {
            case EXECUTABLE_TARGET:
                target = new ExecutableTarget(targetName, this);
                break;
            case LIBRARY_TARGET:
                target = new LibraryTarget(targetName, this);
                break;
            case CUSTOM_TARGET:
                target = new CustomTarget(targetName, this);
                break;
            default:
                throw Error("Unknown target type for target " + targetName);
                break;
        };
        m_targets[targetName] = target;
    }
}

std::list<StringList> MeiqueScript::getTests(const std::string& pattern)
{
    lua_getglobal(m_L, "_meiqueAllTests");
    std::list<StringList> tests;
    readLuaList(m_L, lua_gettop(m_L), tests);
    lua_pop(m_L, 1);

    if (!pattern.empty()) {
        Regex regex(pattern.c_str());
        if (!regex.isValid())
            throw Error("Invalid regular expression.");
        tests.remove_if([&](const StringList& value) {
            return !regex.match(*value.begin());
        });
    }
    return tests;
}

Target* MeiqueScript::getTarget(const std::string& name) const
{
    TargetsMap::const_iterator it = m_targets.find(name);
    if (it == m_targets.end())
        throw Error("Target \"" + name + "\" not found!");
    return it->second;
}

TargetList MeiqueScript::targets() const
{
    TargetList list;
    TargetsMap::const_iterator it = m_targets.begin();
    for (; it != m_targets.end(); ++it)
        list.push_back(it->second);
    return list;
}

static bool checkVersion(const std::string& version)
{
    std::istringstream s(version);
    int component;
    s >> component;
    if (component > MEIQUE_MAJOR_VERSION)
        return false;
    char dot;
    s >> dot;
    if (dot != '.')
        return false;
    s >> component;
    if (component > MEIQUE_MINOR_VERSION)
        return false;
    return true;
}

int requiresMeique(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 1)
        luaError(L, "requiresMeique(version) called with wrong number of arguments.");

    std::string requiredVersion = lua_tocpp<std::string>(L, -1);
    if (!checkVersion(requiredVersion))
        luaError(L, "This project requires a newer version of meique (" + requiredVersion + "), you are using version " MEIQUE_VERSION);
    return 0;
}

StringList MeiqueScript::targetNames()
{
    LuaLeakCheck(m_L);

    lua_getglobal(m_L, "_meiqueAllTargets");
    StringList targets;
    readLuaTableKeys(m_L, lua_gettop(m_L), targets);
    lua_pop(m_L, 1);
    return targets;
}

struct StrFilter
{
    StrFilter(const std::string& garbage) : m_garbage(garbage) {}
    void filter(std::string& str)
    {
        std::istringstream s(str);
        std::string token;
        std::string result;
        while (s) {
            s >> token;
            if (!s)
                break;
            if (token.find(m_garbage) == 0) {
                token.erase(0, m_garbage.size());
                result += ' ';
                result += token;
            } else {
                Debug() << "Discarding \"" << token << "\" from pkg-config link libraries.";
            }
        }
        trim(result);
        str = result;
    }
    std::string m_garbage;
};

int findPackage(lua_State* L)
{
    const char PKGCONFIG[] = "pkg-config";
    int nargs = lua_gettop(L);
    if (nargs < 1 || nargs > 3)
        luaError(L, "findPackage(name [, version, flags]) called with wrong number of arguments.");

    const std::string pkgName = lua_tocpp<std::string>(L, 1);
    const std::string version = lua_tocpp<std::string>(L, 2);
    bool optional = lua_tocpp<bool>(L, 3);

    MeiqueScript* script = getMeiqueScriptObject(L);
    MeiqueCache* cache = script->cache();
/*
    // do nothing when showing the help screen
    if (config.action() == MeiqueCache::ShowHelp) {
        lua_settop(L, 0);
        lua_getglobal(L, "_meiqueNone");
        return 1;
    }
*/
    StringMap pkgData = cache->package(pkgName);
    if (pkgData.empty()) {
        // Check if the package exists
        StringList args;
        args.push_back(pkgName);
        // TODO: Interpret >, >=, < and <= from the version expression
        if (!version.empty())
            args.push_back("--atleast-version="+version);
        int retval = OS::exec(PKGCONFIG, args);
        if (!version.empty())
            args.pop_back();
        if (retval) {
            if (!optional) {
                luaError(L, pkgName + " package not found!");
            } else {
                Notice() << "-- " << pkgName << Red << " not found!";
                pkgData["NOT_FOUND"] = "NOT_FOUND"; // dummy data to avoid an empty map
                cache->setPackage(pkgName, pkgData);
                lua_getglobal(L, "_meiqueNone");
                return 1;
            }
        }

        // Get config options
        const char* pkgConfigCmds[] = {"--libs-only-L",
                                    "--libs-only-l",
                                    "--libs-only-other",
                                    "--cflags-only-I",
                                    "--cflags-only-other",
                                    "--modversion"
                                    };
        const char* names[] = {"libraryPaths",
                            "linkLibraries",
                            "linkerFlags",
                            "includePaths",
                            "cflags",
                            "version"
                            };

        StrFilter libFilter("-l");
        StrFilter libPathFilter("-L");
        StrFilter includeFilter("-I");
        StrFilter* filters[] = {
                                &libPathFilter,
                                &libFilter,
                                0,
                                &includeFilter,
                                0,
                                0,
                            };
        const int N = sizeof(pkgConfigCmds)/sizeof(const char*);
        assert(sizeof(pkgConfigCmds) == sizeof(names));
        for (int i = 0; i < N; ++i) {
            std::string output;
            args.push_back(pkgConfigCmds[i]);
            OS::exec("pkg-config", args, &output);
            args.pop_back();
            trim(output);
            if (filters[i])
                filters[i]->filter(output);
            pkgData[names[i]] = output;
        }
        // Store pkg information
        cache->setPackage(pkgName, pkgData);
        Notice() << "-- " << pkgName << Green << " found!" << NoColor << " (" << pkgData["version"] << ')';
    }

    if (pkgData.size() < 2) { // optimal not found packages has size=1, a NOT_FOUND entry.
        lua_getglobal(L, "_meiqueNone");
    } else {
        lua_settop(L, 0);
        createLuaTable(L, pkgData);
        // do the magic!
        lua_getglobal(L, "_meiqueNotNone");
        lua_setmetatable(L, -2);
    }

    return 1;
}

int copyFile(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs < 1 || nargs > 2)
        luaError(L, "copyFile(input, output) called with wrong number of arguments.");

    lua_getglobal(L, "currentDir");
    lua_call(L, 0, 1);
    std::string currentDir = lua_tocpp<std::string>(L, -1);
    lua_pop(L, 1);

    MeiqueScript* script = getMeiqueScriptObject(L);
    std::string inputArg = lua_tocpp<std::string>(L, -nargs);
    std::string input = OS::normalizeFilePath(script->sourceDir() + currentDir + inputArg);
    std::string output = OS::normalizeFilePath(script->buildDir() + currentDir + (nargs == 1 ? inputArg : lua_tocpp<std::string>(L, -1)));

    if (!script->cache()->isHashGroupOutdated(output, input))
        return 0;

    OS::mkdir(OS::dirName(output));
    std::ifstream source(input.c_str(), std::ios::binary);
    std::ofstream dest(output.c_str(), std::ios::binary);
    dest << source.rdbuf();
    script->cache()->updateHashGroup(output, input);
    return 0;
}

int configureFile(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 2)
        luaError(L, "configureFile(input, output) called with wrong number of arguments.");

    lua_getglobal(L, "currentDir");
    lua_call(L, 0, 1);
    std::string currentDir = lua_tocpp<std::string>(L, -1);
    lua_pop(L, 1);

    // Configure anything when in the help screen
    MeiqueScript* script = getMeiqueScriptObject(L);

/*
    if (script->config().action() == MeiqueCache::ShowHelp) {
        lua_settop(L, 0);
        lua_getglobal(L, "_meiqueNotNone");
        return 1;
    }
*/
    std::string input = OS::normalizeFilePath(script->sourceDir() + currentDir + lua_tocpp<std::string>(L, -2));
    std::string output = OS::normalizeFilePath(script->buildDir() + currentDir + lua_tocpp<std::string>(L, -1));

    if (!script->cache()->isHashGroupOutdated(input, output))
        return 0;

    OS::mkdir(OS::dirName(output));

    std::ifstream in(input.c_str());
    if (!in)
        luaError(L, "Can't read file: " + input);
    std::ofstream out(output.c_str());

    Regex regex("@([^@]+)@");
    std::string line;
    while (in) {
        std::getline(in, line);
        unsigned pos = 0;
        while (pos < line.size()) {
            if (regex.match(line, pos)) {
                std::pair<int, int> idx = regex.group(1);
                lua_getglobal(L, line.substr(pos + idx.first, idx.second).c_str());
                std::string value = lua_tocpp<std::string>(L, -1);
                lua_pop(L, 1);
                line.replace(pos + idx.first - 1, idx.second + 2, value);
                pos += idx.first + value.size();
            } else {
                break;
            }
        }
        out << line << std::endl;
    }

    out.close();
    script->cache()->updateHashGroup(output, input);
    return 0;
}

void MeiqueScript::enableScope(const std::string& scopeName)
{
/*
    // Don't enable any scopes when in help screen
    if (m_cache.action() == MeiqueCache::ShowHelp)
        return;
*/
    lua_State* L = luaState();
    lua_getglobal(L, "_meiqueNotNone");
    lua_setglobal(L, scopeName.c_str());
}

void MeiqueScript::enableBuitinScopes()
{
/*
    // Don't enable any scopes when in help screen
    if (m_cache.action() == MeiqueCache::ShowHelp)
        return;
*/
    StringList scopes = m_cache->scopes();
    if (scopes.empty()) {
        // Enable debug/release scope
        scopes.push_back(m_cache->buildType() == MeiqueCache::Debug ? "DEBUG" : "RELEASE");
        // Enable compiler scope
        std::string compiler = m_cache->compiler()->name();
        std::transform(compiler.begin(), compiler.end(), compiler.begin(), ::toupper);
        scopes.push_back(compiler.c_str());
        // Enable OS scopes
        StringList osVars = OS::getOSType();
        for (StringList::iterator it = osVars.begin(); it != osVars.end(); ++it)
            scopes.push_back(it->c_str());
        m_cache->setScopes(scopes);
    }
    StringList::const_iterator it = scopes.begin();
    for (; it != scopes.end(); ++it)
        enableScope(*it);
}

int option(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs < 2 || nargs > 3)
        luaError(L, "option(name, description [, defaultValue]) called with wrong number of arguments.");
    if (nargs == 2)
        lua_pushnil(L);

    std::string name = lua_tocpp<std::string>(L, 1);
    std::string description = lua_tocpp<std::string>(L, 2);

    if (name.empty())
        luaError(L, "An option MUST have a name.");
    if (description.empty())
        luaError(L, "Be nice and put a description for the option \"" + name + "\" :-).");

    MeiqueScript* script = getMeiqueScriptObject(L);
    std::string optionValue = script->cache()->userOption(name);

    if (optionValue.empty()) {
        // get options table
        lua_getfield(L, LUA_REGISTRYINDEX, MEIQUEOPTIONS_KEY);

        // Create table {description, defaultValue}
        lua_createtable(L, 2, 0);
        lua_pushvalue(L, 2);
        lua_rawseti(L, -2, 1);
        lua_pushvalue(L, 3);
        lua_rawseti(L, -2, 2);

        // to options[name] = {description, defaultValue}
        lua_setfield(L, -2, name.c_str());

        if (script->commandLine()) {
            bool valueFound;
            optionValue = script->commandLine()->arg(name, lua_tocpp<std::string>(L, 3), &valueFound);
            // option provided by the user but without a value, probably a boolean option, sets it to true.
            if (valueFound && optionValue.empty())
                optionValue = "true";
        }

        lua_settop(L, 0);
    }

    // Create return object, a table with a field "value"
    lua_createtable(L, 0, 1);
    lua_pushstring(L, optionValue.c_str());
    lua_setfield(L, -2, "value");

    // Decide if the we will return a valid scope or not.
    // options setted to false
    std::transform(optionValue.begin(), optionValue.end(), optionValue.begin(), ::tolower);

    if (optionValue == "false" || optionValue == "off" || optionValue.empty())
        lua_getglobal(L, "_meiqueNone");
    else
        lua_getglobal(L, "_meiqueNotNone");

    lua_setmetatable(L, -2);
    script->cache()->setUserOptionValue(name, optionValue);

    return 1;
}

int meiqueAutomoc(lua_State* L)
{
    static std::string mocPath;
    if (mocPath.empty()) {
        const StringList args = {"--variable=moc_location", "QtCore"};
        OS::exec("pkg-config", args, &mocPath);
        if (mocPath.empty()) {
            Warn() << "moc not found via pkg-config, trying to use \"rcc\".";
            mocPath = "moc";
        }
    }

    // possible extensions for header files
    static const char* headerExts[] = {
        "h", "hpp", "hxx", "H", 0
    };

    // get target name
    lua_getfield(L, -1, "_name");
    std::string targetName = lua_tocpp<std::string>(L, -1);
    lua_pop(L, 1);

    MeiqueScript* script = getMeiqueScriptObject(L);
    Target* target = script->getTarget(targetName);
    OS::mkdir(target->directory());

    StringList files = target->files();
    if (!files.size())
        return 0;

    std::string srcDir = script->sourceDir() + target->directory();
    std::string binDir = script->buildDir() + target->directory();
    // search what files need to be moced
    StringList::const_iterator it = files.begin();
    Regex regex("# *include +[\"<]([^ ]+\\.moc)[\">]");
    for (; it != files.end(); ++it) {
        std::string filePath = srcDir + *it;
        std::ifstream f(filePath.c_str(), std::ios_base::in);
        std::string line;
        while (f && !f.eof()) {
            std::getline(f, line);
            if (regex.match(line)) {
                // check if the header exists
                std::string headerBase = *it;
                size_t dotIdx = headerBase.find_last_of('.');
                if (dotIdx == std::string::npos)
                    break;
                headerBase.erase(dotIdx + 1);
                headerBase = srcDir + headerBase;
                std::string headerPath;
                for (int i = 0; headerExts[i]; ++i) {
                    std::string test(headerBase + headerExts[i]);
                    if (OS::fileExists(test))
                        headerPath = test;
                }

                if (headerPath.empty()) {
                    Warn() << "Found moc include but can't deduce the header file for " << srcDir << *it;
                    break;
                }

                std::string mocFilePath = binDir + regex.group(1, line);
                if (!OS::fileExists(mocFilePath) || script->cache()->isHashGroupOutdated(headerPath)) {
                    StringList args;
                    args.push_back("-o");
                    args.push_back(OS::normalizeFilePath(mocFilePath));
                    args.push_back(OS::normalizeFilePath(headerPath));
                    Notice() << Blue << "Running moc for header of " << *it;
                    if (!OS::exec(mocPath, args))
                        script->cache()->updateHashGroup(headerPath);
                    else
                        luaError(L, "Error running moc for file " + headerPath);
                }
            }
        }
    }
    return 0;
}

int meiqueQtResource(lua_State* L)
{
    static std::string rccPath;
    if (rccPath.empty()) {
        const StringList args = {"--variable=rcc_location", "QtCore"};
        OS::exec("pkg-config", args, &rccPath);
        if (rccPath.empty()) {
            Warn() << "rcc not found via pkg-config, trying to use \"rcc\".";
            rccPath = "rcc";
        }
    }

    // get target name
    lua_getfield(L, -1, "_name");
    std::string targetName = lua_tocpp<std::string>(L, -1);
    lua_pop(L, 1);

    MeiqueScript* script = getMeiqueScriptObject(L);
    Target* target = script->getTarget(targetName);
    std::string srcDir = script->sourceDir() + target->directory();
    std::string binDir = script->buildDir() + target->directory();
    OS::mkdir(binDir);

    lua_getfield(L, -1, "_qrcFiles");
    StringList files;
    StringList cppFiles;
    readLuaList(L, lua_gettop(L), files);
    lua_pop(L, 1);

    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string qrcFile = OS::normalizeFilePath(srcDir + *it);
        std::string cppFile = OS::normalizeFilePath(binDir + *it + ".cpp");
        cppFiles.push_back(cppFile);

        if (!OS::fileExists(cppFile) || script->cache()->isHashGroupOutdated(qrcFile)) {
            StringList args;
            args.push_back("-o");
            args.push_back(OS::normalizeFilePath(cppFile));
            args.push_back(OS::normalizeFilePath(qrcFile));

            if (!OS::exec(rccPath, args))
                script->cache()->updateHashGroup(qrcFile);
            else
                luaError(L, "Error running rcc on file " + qrcFile);
        }
    }
    target->addFiles(cppFiles);
    return 0;
}

StringList MeiqueScript::projectFiles()
{
    StringList projectFiles;
    projectFiles.push_back("meique.lua");
    lua_getglobal(m_L, "_meiqueProjectFiles");
    readLuaList(m_L, lua_gettop(m_L), projectFiles);
    return projectFiles;
}
