#include "compilabletarget.h"
#include "luacpputil.h"
#include "compiler.h"
#include "logger.h"
#include "config.h"
#include "os.h"
#include "filehash.h"
#include "compileroptions.h"

CompilableTarget::CompilableTarget(const std::string& targetName, MeiqueScript* script)
    : Target(targetName, script), m_compilerOptions(0)
{
}

CompilableTarget::~CompilableTarget()
{
    delete m_compilerOptions;
}

void CompilableTarget::doRun(Compiler* compiler)
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);

    if (files.empty())
        Error() << "Compilable target '" << name() << "' has no files!";

    if (!m_compilerOptions)
        fillCompilerOptions();

    std::string sourceDir = config().sourceRoot() + directory();

    bool needLink = false;
    StringList objects;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string source = sourceDir + *it;
        std::string output = *it + ".o";

        std::string hash = FileHash(source).toString();
        if (!OS::fileExists(output) || hash != config().fileHash(source)) {
            if (!compiler->compile(source, output, m_compilerOptions))
                Error() << "Compilation fail!";
            needLink = true;
        }
        config().setFileHash(source, hash);
        objects.push_back(output);
    }

    if (needLink)
        compiler->link(name(), objects);
    // send them to the compiler
}

void CompilableTarget::fillCompilerOptions()
{
    m_compilerOptions = new CompilerOptions;
    getLuaField("_packages");
    lua_State* L = luaState();
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {

        StringMap map;
        readLuaTable<StringMap>(L, lua_gettop(L), map);

        m_compilerOptions->addIncludePath(map["includePaths"]);
        m_compilerOptions->addFlag(map["cflags"]);
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }


}

