//
// Class for storing round-level information
//

#include <deque>
#include <array>
#include <string>
#include <algorithm>
#include <cassert>
#include <vector>
#include <iostream>

// *** All utilities defined in terms of p1's gain ***

constexpr int MAX_CHILD_STATES = 5;
constexpr int NUM_HANDS = 10;
constexpr float REGRET_INIT = 0.10;


// this represents all possible optimizable states in a history
// ie this is the number of hands the player to act could have
// that combined with the game history is everything player to act knows
struct InfoSets {
    enum class Type {
        Continue,
        Fold,
        Showdown,
        Invalid,
    };

    // whether this is p1 to act or p2 (affects interpretation of utils, and card selection)
    bool is_p1;

    // the number of valid child states
    int num_children;

    // the children of this node
    std::array<InfoSets*, MAX_CHILD_STATES> children;

    // sum of child regret
    std::array<std::array<float, MAX_CHILD_STATES>, NUM_HANDS> sum_child_regr;

    // whether each child state is terminal and the associated util
    std::array<Type, MAX_CHILD_STATES> child_states;
    std::array<float, MAX_CHILD_STATES> terminal_util;

    inline float& get_regr(const int card, const int child_idx) {
        return sum_child_regr[card][child_idx];
    }

    // needed to default initialize arrays
    InfoSets() : num_children(0) {
        for (int card = 0; card < NUM_HANDS; card++)
            sum_child_regr[card].fill(REGRET_INIT);
        children.fill(nullptr);
        child_states.fill(Type::Invalid);
    }

    void add_showdown(float util) {
        child_states[num_children] = Type::Showdown;
        terminal_util[num_children++] = util;
    }

    void add_fold(float util) {
        child_states[num_children] = Type::Fold;
        terminal_util[num_children++] = util;
    }

    void add_child(InfoSets* child) {
        child_states[num_children] = Type::Continue;
        children[num_children++] = child;
    }

    float get_util(int card1, int card2, bool p1_wins, float prob_in) {
        const int info_state_card = is_p1 ? card1 : card2;

        // strategy is just normalized sum_child_regrs (for this state)
        float sum_regr = 0.0;
        for (int child_idx = 0; child_idx < num_children; child_idx++) {
            sum_regr += get_regr(info_state_card, child_idx);
        }

        // util is a weighted avg of child utilities
        float util;
        std::array<float, MAX_CHILD_STATES> child_utils;
        for (int child_idx = 0; child_states[child_idx] != Type::Invalid; child_idx++) {
            const float strat_prob = get_regr(info_state_card, child_idx) / sum_regr;

            switch (child_states[child_idx]) {
            case Type::Continue:
                child_utils[child_idx] = children[child_idx]->get_util(card1, card2, p1_wins, prob_in * strat_prob);
                break;
            case Type::Fold:
                child_utils[child_idx] = terminal_util[child_idx] * (is_p1 ? -1.0 : 1.0);
                break;
            case Type::Showdown:
                child_utils[child_idx] = terminal_util[child_idx] * (p1_wins ? 1.0 : -1.0);
                break;
            }

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
        for (int child_idx = 0; num_children; child_idx++) {
            const float regr = child_utils[child_idx] - util;

            if (is_p1)
                get_regr(info_state_card, child_idx) += regr > 0.0 ? regr * prob_in : 0.0;
            else
                get_regr(info_state_card, child_idx) -= regr < 0.0 ? regr * prob_in : 0.0;
        }

        return util;
    }

    // returns size, just for comparison
    int size_bytes() {
        int sz = sizeof(InfoSets);

        for (int child_idx = 0; num_children; child_idx++) {
            sz += children[child_idx]->size_bytes();
        }

        return sz;
    }

    // prints the strategy in this case
    void print_strat(const std::string& history = "") {
        using std::cout;

        if (is_p1)
            cout << "P1: ";
        else
            cout << "P2: ";

        if (history == "")
            cout << "To open: \n";
        else
            cout << "Facing " << history << ": \n";

        for (int card = 0; card < NUM_HANDS; card++) {
            cout << "\tWith " << card << ":\n";

            // strategy is just normalized sum_child_regrs (for this state)
            float sum_regr = 0.0;
            for (int child_idx = 0; child_idx < num_children; child_idx++) {
                sum_regr += get_regr(card, child_idx);
            }

            for (int child_idx = 0; child_idx < num_children; child_idx++) {
                const float pct = (get_regr(card, child_idx) / sum_regr) / 100.0;

                switch (child_states[child_idx]) {
                case Type::Fold:
                    cout << "Fold " << pct << "% of the time\n";
                    break;
                case Type::Showdown:
                    cout << "Call " << pct << "% of the time\n";
                    break;
                case Type::Continue:
                    cout << "Bet sz " << child_idx << " " << pct << "% of the time\n";
                    break;
                }
            }
        }

        cout << "\n";

        for (int child_idx = 0; child_idx < num_children; child_idx++) {
            if (child_states[child_idx] != Type::Continue) continue;

            const std::string next_history = history + " bet " + std::to_string(child_idx);

            children[child_idx]->print_strat(next_history);
        }
    }
};

class Game {

    float ante, stack_sz;

    // the initial probability for each hand per player
    std::array<float, NUM_HANDS> starting_prob1, starting_prob2;

    // mapping of hand -> ranking where ranking1 > ranking2 iff hand1 beats hand2
    std::array<int, NUM_HANDS> hand_rankings;

    // the game tree, ie game_tree[a][b] is the Node where p1 has a and p2 has b
    InfoSets* root;

public:
    using Bet = std::vector<float>;

    // assumed that a check ends the game
    // p1_mip: amount that p1 has put in the pot
    // p2_mip: amount that p2 has put in the pot
    // p1_wins: whether p1 wins in this game state
    // p1_to_act: whether p1 or p2 are acting in this state
    // rem_bets: deque of the remaining rounds of bet szs (all defined in terms of % of pot)
    InfoSets* make_child(float p1_mip, float p2_mip, bool p1_to_act, const std::deque<Bet>& rem_bets) {
        InfoSets* ret = new InfoSets;

        const float mip = p1_mip + p2_mip + ante;

        // terminal state: fold
        const float fold_util = p1_to_act ? -1.0 * p1_mip - ante : p2_mip + ante;
        ret->add_fold(fold_util);

        // all-in catch case, can only call or fold and the util is clipped on a call
        if (p1_mip > stack_sz || p2_mip > stack_sz) {
            const float call_all_in_util = stack_sz + ante;
            ret->add_showdown(call_all_in_util);
            return ret;
        }

        // terminal state: call
        const float call_util = p1_to_act ? p2_mip + ante : p1_mip + ante;
        ret->add_showdown(call_util);

        const Bet& this_bets = rem_bets.front();

        // this is quite inefficient, but this is the cold path anyways
        auto next_round_rem_bets = rem_bets;
        next_round_rem_bets.pop_front();

        // bet case(s)
        for (int bet_idx = 0; bet_idx < this_bets.size(); bet_idx++) {
            const float next_bet = mip * this_bets[bet_idx];

            InfoSets* child;

            if (p1_to_act)
                child = make_child(next_bet, p2_mip, false, next_round_rem_bets);
            else
                child = make_child(p1_mip, next_bet, true, next_round_rem_bets);

            ret->add_child(child);
        }

        return ret;
    }

    // special case: facing a check, you can't fold
    // must be p2 to act
    InfoSets* make_child_no_fold(const std::deque<Bet>& all_bets) {
        InfoSets* ret = new InfoSets;

        // terminal state: check-through
        ret->add_showdown(ante);

        const Bet& this_bets = all_bets.front();

        // this is quite inefficient, but this is the cold path anyways
        auto rem_bets = all_bets;
        rem_bets.pop_front();

        // bet case(s)
        for (int bet_idx = 0; bet_idx < this_bets.size(); bet_idx++) {
            const float next_bet = ante * this_bets[bet_idx];
            InfoSets* child = make_child(0.0, next_bet, true, rem_bets);
            ret->add_child(child);
        }

        return ret;
    }

    // special case: under the gun, you can't fold and a check isn't terminal
    // must be p1 to act
    InfoSets* make_child_check_allowed(const std::deque<Bet>& all_bets) {
        InfoSets* ret = new InfoSets;

        // no immediate terminal states to create

        const Bet& this_bets = all_bets.front();

        // this is quite inefficient, but this is the cold path anyways
        auto rem_bets = all_bets;
        rem_bets.pop_front();

        // check case
        ret->add_child(make_child_no_fold(all_bets));

        // bet case(s)
        for (int bet_idx = 0; bet_idx < this_bets.size(); bet_idx++) {
            const float next_bet = ante * this_bets[bet_idx];
            InfoSets* child = make_child(next_bet, 0.0, false, rem_bets);
            ret->add_child(child);
        }

        return ret;
    }

    Game(
        std::array<float, NUM_HANDS> _starting_prob1,
        std::array<float, NUM_HANDS> _starting_prob2,
        std::array<int, NUM_HANDS> _hand_rankings,
        float _ante, float _stack_sz, std::deque<Bet> bet_sizes)
        : ante(_ante), stack_sz(_stack_sz),
        starting_prob1(_starting_prob1), starting_prob2(_starting_prob2),
        hand_rankings(_hand_rankings) {
        root = make_child_check_allowed(bet_sizes);
    }

    // returns the util of this state
    float train_state(int card1, int card2, float prob) {
        assert(card1 != card2);

        const bool p1_wins = hand_rankings[card1] > hand_rankings[card2];

        return root->get_util(card1, card2, p1_wins, prob);
    }

    // returns the util after training one iteration
    float train() {
        float util = 0.0;

        for (int card1 = 0; card1 < NUM_HANDS; card1++) {
            for (int card2 = 0; card2 < NUM_HANDS; card2++) {
                if (card1 == card2) continue;

                const float prob = starting_prob1[card1] * starting_prob2[card2];

                util += prob * train_state(card1, card2, prob);
            }
        }

        return util;
    }

    void print() {
        root->print_strat();
    }
};