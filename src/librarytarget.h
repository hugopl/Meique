#ifndef LIBRARYTARGET_H
#define LIBRARYTARGET_H
#include "compilabletarget.h"

class LibraryTarget : public CompilableTarget
{
public:
    LibraryTarget(const std::string& targetName, MeiqueScript* script) : CompilableTarget(targetName, script) {}
protected:
    virtual void doRun(Compiler* compiler);
};

#endif
