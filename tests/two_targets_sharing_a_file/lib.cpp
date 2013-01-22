#define API_EXPORT __attribute__ ((visibility("default")))

API_EXPORT int foo(int);

API_EXPORT void justAnotherExportedFunction()
{
    foo(reinterpret_cast<unsigned long>(&foo));
}
