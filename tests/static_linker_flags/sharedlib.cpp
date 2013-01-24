#define API_EXPORT __attribute__ ((visibility("default")))

API_EXPORT int foo()
{
    return 42;
}
