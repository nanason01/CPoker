//
// A (hopefully) simple executable currently mostly to validate that 
// everything else actually compiles
//

#include <iostream>
#include <ratio>

// #include "Configs.h"
// #include "Pack.h"
#include "Bet.h"
#include "Round.h"

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
    using bets = Bet<30, 60, 150>;
    using raises = Bet<300, 500>;

    using round = Round<bets, raises>;

    cout << round::mta<2>() << "\n";  // 0.3
    cout << round::mip<2>() << "\n";  // 1.3

    cout << round::mta<2, 2>() << "\n";  // 3.9
    cout << round::mip<2, 2>() << "\n";  // 5.2

    cout << round::mta<4, 3>() << "\n";  // 10
    cout << round::is_all_in<4, 3>() << "\n";  // 1
}