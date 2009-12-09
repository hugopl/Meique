
#ifndef MEIQUE_H
#define MEIQUE_H
#include "config.h"
#include "meiquescript.h"

class Meique
{
public:
    Meique(int argc, char** argv);
    void exec();
private:
    Config m_config;

    void checkOptionsAgainstArguments(const OptionsMap& options);
};

#endif
