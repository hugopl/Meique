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

    StringList objects;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string source = config().sourceRoot();
        source += *it;
        std::string output = *it + ".o";
        if (!compiler->compile(source, output))
            Error() << "Compilation fail!";
        objects.push_back(output);
    }
    compiler->link(objects);
    // send them to the compiler
}

