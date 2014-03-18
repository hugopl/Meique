#include "os.h"
#include <iostream>

#define TEST(X, Y, EXPECTED) if (!testNormalize((X), (Y), (EXPECTED), __LINE__)) return 1

bool testNormalize(const std::string& path, const std::string& rel, const std::string& expected, unsigned line)
{
    std::string got = OS::relativePath(path, rel);
    if (got != expected) {
        std::cout << "Failed at line " << line << ", " << got << " != " << expected;
        return false;
    }
    return true;
}

int main() {
     TEST("/file", "/a/b/c/", "../../../file");
     TEST("/a/b/c/d/file", "/a/b/c/", "d/file");
     TEST("/a/b/c/file", "/a/b/c/", "file");

     TEST("/file/", "/a/b/c/", "../../../file/");
     TEST("/a/b/c/d/file/", "/a/b/c/", "d/file/");
     TEST("/a/b/c/file/", "/a/b/c/", "file/");

}
