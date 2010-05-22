#ifndef COMPILABLETARGET_H
#define COMPILABLETARGET_H

#include "target.h"

class CompilerOptions;

class CompilableTarget : public Target
{
public:
    CompilableTarget(const std::string& targetName, MeiqueScript* script);
    ~CompilableTarget();
protected:
    void doRun(Compiler* compiler);
private:
    CompilerOptions* m_compilerOptions;
    void fillCompilerOptions();
};

#endif
