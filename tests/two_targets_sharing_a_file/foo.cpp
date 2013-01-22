#include <stdio.h>

#define API_EXPORT __attribute__ ((visibility("default")))

API_EXPORT int foo(int bar)
{
    puts(TARGET_NAME);
    return bar * 2;
}
