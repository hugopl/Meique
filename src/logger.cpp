#include "logger.h"

bool MeiqueError::errorAlreadyset = false;

MeiqueError::MeiqueError()
{
    errorAlreadyset = true;
}

