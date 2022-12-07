//
// A (hopefully) simple executable currently mostly to validate that 
// everything else actually compiles
//

#include <iostream>
#include <ratio>

// #include "Configs.h"
// #include "Pack.h"
// #include "Bet.h"
// #include "Round.h"

template <bool b, int i>
struct Test {
    static_assert(!sizeof(Test), "failure");
};


template <int i>
struct Test<false, i> {
    bool data[i];
    static constexpr int x = 3;

    Test<false, i - 1> child;
};

template <>
struct Test<false, 1> {
    bool data[1];
};

template <long num, long dem>
std::ostream& operator << (std::ostream& os, const std::ratio<num, dem>& v) {
    os << (float)num / (float)dem;

    return os;
}

using std::cout;

int main() {
    Test<false, 4> f_test;

    cout << sizeof(f_test) << "\n";
}