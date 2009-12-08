#include "meique.h"
#include "logger.h"
#include "meiquescript.h"

Meique::Meique(int argc, char** argv) : m_config(argc, argv)
{
}

void Meique::exec()
{
    MeiqueScript script(m_config);
    script.exec();
}


