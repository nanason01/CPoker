
#include "GameTree.h"

#include <iostream>

using Bet = std::vector<float>;
using GameDef = std::deque<Bet>;

int main() {
    Bet bets = { 2.0, 4.0 }, raises = { 3.0 };

    GameDef game_structure = { bets, raises };

    CardDistro p;
    p.fill(1.0 / NUM_HANDS);

    const float ante = 1.0, stack_sz = 10.0;

    Game g(p, p, ante, stack_sz, game_structure, false);

    std::cout << "tree sz actual: " << g.get_size_bytes() << "\n";
    std::cout << "tree sz needed: " << g.get_size_used() << "\n";

    for (int i = 0; i < 100000; i++)
        g.train();

    g.train();
    g.print();
    auto losses = g.nash_dist();

    std::cout << "util from mes against p1: " << losses.first << "\n";
    std::cout << "util from mes against p2: " << losses.second << "\n";

    float nash_dist = losses.second - losses.first;

    std::cout << "nash dist: " << nash_dist << "\n";
}
