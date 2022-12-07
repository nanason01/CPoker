
#include "Round.h"

#include <deque>
#include <vector>

using Bet = std::vector<float>;
using GameDef = std::deque<Bet>;

int main() {
    Bet bets1 = { 1.0 };

    GameDef game_structure = { bets1 };

    std::array<float, NUM_HANDS> p;
    p.fill(1.0 / NUM_HANDS);

    std::array<int, NUM_HANDS> hand_ranks;
    for (int i = 0; i < NUM_HANDS; i++)
        hand_ranks[ i ] = i;

    const float ante = 1.0, stack_sz = 10.0;

    Game g(
        p,
        p,
        hand_ranks,
        ante,
        stack_sz,
        game_structure
    );

    for (int i = 0; i < 10000; i++)
        g.train();

    g.print();
}
