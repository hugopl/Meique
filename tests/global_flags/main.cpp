#include <iostream>

using namespace std;

int main() {
    cout <<
#if TEST_WILL_PASS1 && TEST_WILL_PASS2 && TEST_WILL_PASS3
    "Pass"
#else
    "Fail"
#endif
    << endl;
}
