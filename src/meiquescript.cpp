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
#include "compiler.h"
#include "meiquecache.h"
#include "logger.h"
#include "luacpputil.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"
#include "os.h"
#include "stdstringsux.h"
#include "meiqueregex.h"
#include "meiqueversion.h"

enum TargetTypes {
    EXECUTABLE_TARGET = 1,
    LIBRARY_TARGET,
    CUSTOM_TARGET
};

// Key used to store the meique script object on lua registry
#define MEIQUESCRIPTOBJ_KEY "MeiqueScript"

static int requiresMeique(lua_State* L);
static int findPackage(lua_State* L);
static int copyFile(lua_State* L);
static int configureFile(lua_State* L);
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
    delete m_cache;
}

void MeiqueScript::populateOptionsValues()
{
    LuaLeakCheck(m_L);
    lua_getglobal(m_L, "_meiqueOptionsValues");
    LuaAutoPop autoPop(m_L);
    int idx = lua_gettop(m_L);

    if (m_cmdLine) {
        StringMap args = m_cmdLine->args();
        // Remove meique options from args.
        // TODO: The list of arguments should go to a proper global place some day
        eraseIf(args, [](const StringMap::value_type& pair) {
            static const char* options[] = {
                "debug",
                "install-prefix",
                "release"
            };
            return std::find(options, options + sizeof(options)/sizeof(char*), pair.first);
        });
        m_cache->setUserOptionsValues(args);
    }

    for (const auto& pair : m_cache->userOptionsValues()) {
        lua_pushstring(m_L, pair.first.c_str());
        lua_pushstring(m_L, pair.second.c_str());
        lua_rawset(m_L, idx);
    }
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
    populateOptionsValues();

    translateLuaError(m_L, luaL_loadfile(m_L, m_scriptName.c_str()), m_scriptName);

    luaPCall(m_L, 0, 0, m_scriptName);
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
    lua_register(m_L, "meiqueSourceDir", &meiqueSourceDir);
    lua_register(m_L, "meiqueBuildDir", &meiqueBuildDir);
    lua_register(m_L, "_meiqueAutomoc", &meiqueAutomoc);
    lua_register(m_L, "_meiqueQtResource", &meiqueQtResource);
    lua_settop(m_L, 0);

    // Export MeiqueScript class to lua registry
    lua_pushlightuserdata(m_L, (void*) this);
    lua_setfield(m_L, LUA_REGISTRYINDEX, MEIQUESCRIPTOBJ_KEY);
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

StringMap MeiqueScript::getOptionsValues()
{
    LuaLeakCheck(m_L);
    lua_getglobal(m_L, "_meiqueOptionsValues");
    StringMap options;
    readLuaTable(m_L, lua_gettop(m_L), options);
    lua_pop(m_L, 1);
    return options;
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

    if (OS::timestampCompare(output, input) > 0)
        return 0;

    OS::mkdir(OS::dirName(output));
    std::ifstream source(input.c_str(), std::ios::binary);
    std::ofstream dest(output.c_str(), std::ios::binary);
    dest << source.rdbuf();
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
    std::string input = OS::normalizeFilePath(script->sourceDir() + currentDir + lua_tocpp<std::string>(L, -2));
    std::string output = OS::normalizeFilePath(script->buildDir() + currentDir + lua_tocpp<std::string>(L, -1));

    if (OS::timestampCompare(output, input) > 0)
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
    return 0;
}

void MeiqueScript::enableScope(const std::string& scopeName)
{
    lua_State* L = luaState();
    lua_getglobal(L, "_meiqueNotNone");
    lua_setglobal(L, scopeName.c_str());
}

void MeiqueScript::enableBuitinScopes()
{
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

int meiqueAutomoc(lua_State* L)
{
    LuaLeakCheck(L);

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

    std::string directory = luaGetField<std::string>(L, "_dir");

    MeiqueScript* script = getMeiqueScriptObject(L);
    OS::mkdir(script->buildDir() + directory);

    std::string srcDir = script->sourceDir() + directory;
    std::string binDir = script->buildDir() + directory;
    // search what files need to be moced
    Regex regex("# *include +[\"<]([^ ]+\\.moc)[\">]");
    for (const std::string& file : luaGetField<StringList>(L, "_files")) {
        std::string filePath = srcDir + file;
        std::ifstream f(filePath.c_str(), std::ios_base::in);
        std::string line;
        while (f && !f.eof()) {
            std::getline(f, line);
            if (regex.match(line)) {
                // check if the header exists
                std::string headerBase = file;
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
                    Warn() << "Found moc include but can't deduce the header file for " << srcDir << file;
                    break;
                }

                std::string mocFilePath = binDir + regex.group(1, line);
                if (OS::timestampCompare(headerPath, mocFilePath) < 0 ) {
                    StringList args;
                    args.push_back("-o");
                    args.push_back(OS::normalizeFilePath(mocFilePath));
                    args.push_back(OS::normalizeFilePath(headerPath));
                    Notice() << Blue << "Running moc for header of " << file;
                    if (OS::exec(mocPath, args))
                        luaError(L, "Error running moc for file " + headerPath);
                }
            }
        }
    }

    return 0;
}

int meiqueQtResource(lua_State* L)
{
    LuaLeakCheck(L);

    static std::string rccPath;
    if (rccPath.empty()) {
        const StringList args = {"--variable=rcc_location", "QtCore"};
        OS::exec("pkg-config", args, &rccPath);
        if (rccPath.empty()) {
            Warn() << "rcc not found via pkg-config, trying to use \"rcc\".";
            rccPath = "rcc";
        }
    }

    MeiqueScript* script = getMeiqueScriptObject(L);
    std::string directory = luaGetField<std::string>(L, "_dir");
    std::string srcDir = script->sourceDir() + directory;
    std::string binDir = script->buildDir() + directory;
    OS::mkdir(binDir);

    StringList cppFiles;
    for (const std::string& file : luaGetField<StringList>(L, "_qrcFiles")) {
        std::string qrcFile = OS::normalizeFilePath(srcDir + file);
        std::string cppFile = OS::normalizeFilePath(binDir + file + ".cpp");
        cppFiles.push_back(cppFile);

        if (OS::timestampCompare(qrcFile, cppFile) < 0) {
            StringList args;
            args.push_back("-o");
            args.push_back(OS::normalizeFilePath(cppFile));
            args.push_back(OS::normalizeFilePath(qrcFile));

            Notice() << Blue << "Running qrc for " << file;
            if (OS::exec(rccPath, args))
                luaError(L, "Error running rcc on file " + qrcFile);
        }
    }

    if (!cppFiles.empty()) {
        try {
            lua_getfield(L, -1, "addFiles");
            lua_pushvalue(L, -2);
            lua_pushstring(L, join(cppFiles, " ").c_str());
            LuaAutoPop autoPop(L);
            luaPCall(L, 2, 0);
        } catch (const Error& e) {
            luaError(L, e.description());
        }
    }

    return 0;
}

StringList MeiqueScript::projectFiles()
{
    LuaLeakCheck(m_L);

    StringList projectFiles;
    projectFiles.push_back("meique.lua");
    lua_getglobal(m_L, "_meiqueProjectFiles");
    readLuaList(m_L, lua_gettop(m_L), projectFiles);
    lua_pop(m_L, 1);
    return projectFiles;
}

void MeiqueScript::luaPushTarget(const char* target)
{
    lua_getglobal(m_L, "_meiqueAllTargets");
    lua_getfield(m_L, -1, target);
    lua_remove(m_L, -2);
}

void MeiqueScript::installTargets(const StringList& targets)
{
    LuaLeakCheck(m_L);

    for (const std::string& target : (targets.empty() ? targetNames() : targets)) {
        luaPushTarget(target);
        LuaAutoPop autoPop(m_L);

        std::string directory = luaGetField<std::string>(m_L, "_dir");

        lua_getfield(m_L, -1, "_installFiles");
        std::list<StringList> installDirectives;
        readLuaList(m_L, lua_gettop(m_L), installDirectives);
        lua_pop(m_L, 1);

        if (installDirectives.empty())
            continue;

        Notice() << Cyan << "Installing " << target << "...";

        for (const StringList& directive : installDirectives) {
            const int directiveSize = directive.size();
            if (!directiveSize)
                continue;

            const std::string destDir = OS::normalizeDirPath(m_cache->installPrefix() + directive.front());
            const std::string srcDir = m_cache->sourceDir() + directory;

            if (directiveSize == 1) { // Target installation
                std::string targetFile = OS::normalizeFilePath(directory + luaGetField<std::string>(m_L, "_output"));
                OS::install(targetFile, destDir);
            } else if (directiveSize > 1) { // custom file install
                for (const std::string& item : directive)
                    OS::install(srcDir + item, destDir);
            }
        }
    }
}

void MeiqueScript::uninstallTargets(const StringList& targets)
{
    LuaLeakCheck(m_L);

    for (const std::string& target : (targets.empty() ? targetNames() : targets)) {
        luaPushTarget(target);
        LuaAutoPop autoPop(m_L);

        lua_getfield(m_L, -1, "_installFiles");
        std::list<StringList> installDirectives;
        readLuaList(m_L, lua_gettop(m_L), installDirectives);
        lua_pop(m_L, 1);

        if (installDirectives.empty())
            continue;

        Notice() << Cyan << "Uninstalling " << target << "...";

        for (const StringList& directive : installDirectives) {
            const int directiveSize = directive.size();
            if (!directiveSize)
                continue;

            const std::string destDir = OS::normalizeDirPath(m_cache->installPrefix() + directive.front());

            if (directiveSize == 1) { // Target installation
                OS::uninstall(destDir + luaGetField<std::string>(m_L, "_output"));
            } else if (directiveSize > 1) { // custom file install
                for (const std::string& item : directive)
                    OS::uninstall(destDir + OS::baseName(item));
            }
        }
    }
}

void MeiqueScript::cleanTargets(const StringList& targets)
{
    LuaLeakCheck(m_L);

    for (const std::string& target : (targets.empty() ? targetNames() : targets)) {
        luaPushTarget(target);
        LuaAutoPop autoPop(m_L);

        std::string directory = luaGetField<std::string>(m_L, "_dir");
        StringList files = luaGetField<StringList>(m_L, "_files");
        Compiler* compiler = m_cache->compiler();
        for (const std::string& file : files) {
            std::string objFile = compiler->nameForObject(file, target);
            if (objFile[0] != '/')
                objFile.insert(0, m_buildDir + directory);
            OS::rm(objFile);
        }
    }
}

StringList MeiqueScript::getTargetIncludeDirectories(const std::string& target)
{
    LuaLeakCheck(m_L);

    luaPushTarget(target);
    LuaAutoPop autoPop(m_L);

    StringList list;
    if (luaGetField<int>(m_L, "_type") == CUSTOM_TARGET)
        return list;

    // explicit include directories
    list = luaGetField<StringList>(m_L, "_incDirs");
    list.sort();

    lua_getfield(m_L, -1, "_packages");
    // loop on all used packages
    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    while (lua_next(m_L, tableIndex) != 0) {
        if (lua_istable(m_L, -1)) {
            StringMap map;
            readLuaTable(m_L, lua_gettop(m_L), map);
            StringList incDirs(split(map["includePaths"]));
            list.merge(incDirs);
        }
        lua_pop(m_L, 1); // removes 'value'; keeps 'key' for next iteration
    }
    lua_pop(m_L, 1);

    return list;
}

void MeiqueScript::dumpProject(std::ostream& output)
{
    LuaLeakCheck(m_L);

    const std::string sourceDir = m_cache->sourceDir();
    output << "Project: " << OS::baseName(sourceDir) << std::endl;

    // Project files
    for (std::string& file : projectFiles())
        output << "ProjectFile: " << OS::normalizeFilePath(sourceDir + file) << "\n";

    for (std::string& target : targetNames()) {
        std::cout << "Target: " << target << std::endl;

        luaPushTarget(target);
        LuaAutoPop autoPop(m_L);

        std::string directory = luaGetField<std::string>(m_L, "_dir");

        for (const std::string& fileName : luaGetField<StringList>(m_L, "_files")) {
            if (fileName.empty())
                continue;
            std::string absPath = fileName[0] == '/' ? fileName : sourceDir + directory + fileName;
            absPath = OS::normalizeFilePath(absPath);
            output << "File: " << absPath << std::endl;
            auto lastDot = absPath.find_last_of(".");
            if (lastDot != std::string::npos) {
                absPath.replace(lastDot, absPath.size() - lastDot, ".h");
                if (OS::fileExists(absPath))
                    output << "File: " << absPath << std::endl;
            }

//          TODO: Defines
        }

        output << "Include: " << OS::normalizeDirPath(sourceDir + directory) << std::endl;
        for (const std::string& inc : getTargetIncludeDirectories(target))
            output << "Include: " << OS::normalizeDirPath(inc) << std::endl;
    }
}

bool MeiqueScript::hasHook(const char *target)
{
    LuaLeakCheck(m_L);

    luaPushTarget(target);
    lua_getfield(m_L, -1, "_preTargetCompileHooks");
    LuaAutoPop autoPop(m_L, 2);

    int tableIndex = lua_gettop(m_L);
    lua_pushnil(m_L);  /* first key */
    if (lua_next(m_L, tableIndex) != 0) {
        lua_pop(m_L, 2);
        return true;
    }
    return false;
}
