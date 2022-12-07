//
// Class for storing round-level information
//

#include <vector>
#include <array>
#include <algorithm>
#include <cassert>

// *** All utilities defined in terms of p1's gain ***

constexpr int MAX_CHILD_STATES = 5;
constexpr int NUM_HANDS = 10;
constexpr float REGRET_INIT = 0.10;


struct Node {
    // whether this is p1 to act or p2 (affects interpretation of utils)
    // whether this is a terminal node (ie fold or showdown)
    bool is_p1, is_terminal;

    // the children of this node
    std::array<Node*, MAX_CHILD_STATES> children;

    // sum of child regret (also a lazy union that front() == util iff is_terminal)
    std::array<float, MAX_CHILD_STATES> sum_child_regr;

    // needed to default initialize arrays
    Node() = default;

    // terminal ctor
    Node(float terminal_util) : is_terminal(true) {
        sum_child_regr.front() = terminal_util;
    }

    // internal ctor
    // children still need to be initialized
    Node(bool _is_p1) : is_p1(_is_p1), is_terminal(false) {
        sum_child_regr.fill(REGRET_INIT);
        children.fill(nullptr);
    }

    float get_util(float prob_in) {
        if (is_terminal) return sum_child_regr.front();
        else assert(children.front());  // must be >= 1 child if not in terminal state

        // strategy is just normalized sum_child_regrs
        const float sum_regr = std::accumulate(sum_child_regr.begin(), sum_child_regr.end(), 0.0);

        // util is a weighted avg of child utilities
        float util;
        std::array<float, MAX_CHILD_STATES> child_utils;
        for (int child_idx = 0; children[child_idx]; child_idx++) {
            const float strat_prob = sum_child_regr[child_idx] / sum_regr;
            child_utils[child_idx] = children[child_idx]->get_util(prob_in * strat_prob);
            util += child_utils[child_idx] * strat_prob;
        }

        // update sum of regrets
        // Regret: the amount you would've benefited from doing an action
        // ie child_util - curr_util
        // Weight by prob_in, intuitively a state that doesn't occur often should have less
        // weight that one that's highly likely. You win a lot if you call all-in and villain
        // has dueces, but if he went all-in, he more likely has at least a decent hand.
        // influence non-negative regrets, where non-negative depends on player perspective
        // p1 and p2 have opposite perspectives, so p1 is trying to maximize util
        // whereas p2 is trying to minimize it.
        // So, p1 will update non-negative regrets whereas p2 will update non-positive regrets
        for (int child_idx = 0; children[child_idx]; child_idx++) {
            const float regr = child_utils[child_idx] - util;

            if (is_p1)
                sum_child_regr[child_idx] += regr > 0.0 ? regr * prob_in : 0.0;
            else
                sum_child_regr[child_idx] -= regr < 0.0 ? regr * prob_in : 0.0;
        }

        return util;
    }
};

class Game {
public:
    using Bet = std::vector<float>;

    Node root;

    // the initial probability for each hand per player
    std::array<float, NUM_HANDS> starting_prob1, starting_prob2;
    // mapping of hand -> ranking where ranking1 > ranking2 iff hand1 beats hand2
    std::array<int, NUM_HANDS> hand_rankings;
    // the game tree, ie game_tree[a][b] is the Node where p1 has a and p2 has b
    std::array<std::array<Node, NUM_HANDS>, NUM_HANDS> game_tree;

    Game(
        std::array<float, NUM_HANDS> _starting_prob1,
        std::array<float, NUM_HANDS> _starting_prob2,
        std::array<int, NUM_HANDS> _hand_rankings,
        float ante, float stack_size, std::vector<Bet> bet_sizes)
        : starting_prob1(_starting_prob1), starting_prob2(_starting_prob2), hand_rankings(_hand_rankings) {

    }
}