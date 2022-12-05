//
// A (hopefully) simple executable currently mostly to validate that 
// everything else actually compiles
//

#include <iostream>
#include <ratio>

#include "Configs.h"
#include "Pack.h"
#include "Bet.h"
#include "Round.h"

template <long num, long dem>
float f(std::ratio<num, dem> r) {
    return (float)num / (float)dem;
}

using std::cout;

int main() {

    using main_round = Round<10, Bet<50, 75, 100>, Bet<200, 600, 700, 800, 900>>;

    main_round::check_valid_idx<>();

    cout << "get_bet: " << f(main_round::get_bet<4, 2>()) << std::endl;
    cout << std::endl;

    cout << "mta: " << f(main_round::mta<4, 3>()) << std::endl;
    cout << "mta: " << f(main_round::mta<4>()) << std::endl;
    cout << "mip: " << f(main_round::mip<4, 3, 1>()) << std::endl;
    cout << "mip: " << f(main_round::mip<4, 2>()) << std::endl;
    cout << "mip: " << f(main_round::mip<3, 1>()) << std::endl;
    cout << "is_all_in: " << main_round::is_all_in<4, 2>() << std::endl;

    cout << "num_bets:\n";
    cout << main_round::count_num_bets<1, 3>() << "\n";

    cout << "child states: " << main_round::num_children<1, 4>() << "\n";
}