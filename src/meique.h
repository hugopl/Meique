
#ifndef MEIQUE_H
#define MEIQUE_H
#include "config.h"

class Meique
{
public:
    Meique(int argc, char** argv);
    void exec();
private:
    Config m_config;
};

#endif
