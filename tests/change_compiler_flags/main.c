#include <stdio.h>

#ifdef THINGS_CHANGED
#define MSG "Changed"
#else
#define MSG "Not changed"
#endif

int main()
{
    puts(MSG);
    return 0;
}
