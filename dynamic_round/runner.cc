
#include "Round.h"

#include <deque>
#include <vector>

using Bet = std::vector<float>;
using GameDef = std::deque<Bet>;

int main() {
    Bet bets1 = { 2.0 };

    GameDef game_structure = { bets1 };

    std::array<float, NUM_HANDS> p1 = { 0.5, 0.0, 0.5 }, p2 = { 0.0, 1.0, 0.0 };

    std::array<int, NUM_HANDS> hand_ranks;
    for (int i = 0; i < NUM_HANDS; i++)
        hand_ranks[i] = i;

    const float ante = 1.0, stack_sz = 10.0;

    Game g(
        p1,
        p2,
        hand_ranks,
        ante,
        stack_sz,
        game_structure
    );

    for (int i = 0; i < 100000; i++)
        g.train();

    float util = g.train();
    g.print();
    auto losses = g.nash_dist();

    std::cout << "util in state: " << util << "\n";
    std::cout << "util from mes against p1: " << losses.first << "\n";
    std::cout << "util from mes against p2: " << losses.second << "\n";

    float nash_dist = losses.second - losses.first;

    std::cout << "nash dist: " << nash_dist << "\n";

}
