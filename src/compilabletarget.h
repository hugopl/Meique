#ifndef COMPILABLETARGET_H
#define COMPILABLETARGET_H
#include "target.h"

class CompilableTarget : public Target
{
public:
    CompilableTarget(const std::string& targetName, MeiqueScript* script);
protected:
    void doRun(Compiler* compiler);
};

#endif
