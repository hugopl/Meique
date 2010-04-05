#ifndef CUSTOMTARGET_H
#define CUSTOMTARGET_H
#include "target.h"

class CustomTarget : public Target
{
public:
    CustomTarget(const std::string& targetName, MeiqueScript* script) : Target(targetName, script) {}
protected:
    virtual void doRun(Compiler* compiler);
};

#endif
