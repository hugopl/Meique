
#include <iostream>
#include "meique.h"
#include "logger.h"

int main(int argc, char** argv)
{
    try {
        Meique meique(argc, argv);
        meique.exec();
    } catch (MeiqueError&) {
        return 1;
    }
    return 0;
}