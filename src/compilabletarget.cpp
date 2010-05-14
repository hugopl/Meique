#include "compilabletarget.h"
#include "luacpputil.h"
#include "compiler.h"
#include "logger.h"
#include "config.h"
#include "os.h"
#include "filehash.h"

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

    std::string sourceDir = config().sourceRoot() + directory();

    bool needLink = false;
    StringList objects;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string source = sourceDir + *it;
        std::string output = *it + ".o";

        std::string hash = FileHash(source).toString();
        if (!OS::fileExists(output) || hash != config().fileHash(source)) {
            if (!compiler->compile(source, output))
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

