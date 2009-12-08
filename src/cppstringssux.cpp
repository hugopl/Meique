#include "cppstringssux.h"

void stringReplace(std::string& str, const std::string& substr, const std::string& replace) {
    size_t n = 0;
    while (true) {
        n = str.find(substr, n);
        if (n == std::string::npos)
            break;
        str.replace(n, substr.length(), replace);
        n += substr.length()+1;
    }
}

