
#include "GameTree.h"

#include <chrono>
#include <iostream>

using Bet = std::vector<float>;
using GameDef = std::deque<Bet>;

int main() {
    Bet bets = { 2.0 };

    GameDef game_structure = { bets };

    CardDistro p;
    p.fill(1.0 / NUM_HANDS);

    const float ante = 1.0, stack_sz = 10.0;

    Game g(p, p, ante, stack_sz, game_structure, true);

    std::cout << "structure,iterations,time,nash_dist\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    auto net_time = std::chrono::duration_cast<std::chrono::milliseconds>(start_time - start_time).count();

    for (int i = 0; i < 100000; i++) {
        g.train();
        if (i % 100 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            net_time += std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

            auto losses = g.nash_dist();

            float nash_dist = losses.second - losses.first;

            std::cout << "complex," << i << "," << net_time << "," << nash_dist << "\n";

            start_time = std::chrono::high_resolution_clock::now();
        }
    }

    //g.train();
    // g.print();
    auto losses = g.nash_dist();


    float nash_dist = losses.second - losses.first;

    std::cout << "nash dist: " << nash_dist << "\n";
}
