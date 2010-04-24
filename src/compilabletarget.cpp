#include "compilabletarget.h"
#include "luacpputil.h"
#include "compiler.h"
#include "logger.h"
#include "config.h"

CompilableTarget::CompilableTarget(const std::string& targetName, MeiqueScript* script) : Target(targetName, script)
{
}

void CompilableTarget::doRun(Compiler* compiler)
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);

    if (files.empty())
        Error() << "Compilable target '" << name() << "' has no files!";

    getLuaField("_dir");
    std::string sourceDir(lua_tocpp<std::string>(luaState(), -1));
    lua_pop(luaState(), 1);
    if (!sourceDir.empty())
        sourceDir += '/';
    sourceDir = config().sourceRoot() + sourceDir;

    StringList objects;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string source = sourceDir + *it;
        std::string output = *it + ".o";
        if (!compiler->compile(source, output))
            Error() << "Compilation fail!";
        objects.push_back(output);
    }
    compiler->link(name(), objects);
    // send them to the compiler
}

