//
// Class for storing round-level information
//

#include <deque>
#include <array>
#include <string>
#include <numeric>
#include <cassert>
#include <vector>
#include <limits>
#include <iostream>

// *** All utilities defined in terms of p1's gain ***

#ifdef DEBUG
#define dprint printf
#else
#define dprint(...)
#endif

constexpr int MAX_CHILD_STATES = 5;
constexpr int NUM_HANDS = 3;
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

    using CardDistro = std::array<float, NUM_HANDS>;

    // the position of this node in the tree
    std::string debug_position = "";

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

    // this is hardcoded for now, but eventually this should read from some central source
    static bool p1_wins(int card1, int card2) {
        return card1 > card2;
    }

    // needed to default initialize arrays
    InfoSets(bool _is_p1) : is_p1(_is_p1), num_children(0) {
        for (int card = 0; card < NUM_HANDS; card++)
            sum_child_regr[card].fill(REGRET_INIT);
        children.fill(nullptr);
        child_states.fill(Type::Invalid);
    }

    void add_showdown(float util) {
        assert(util > 0.0);
        child_states[num_children] = Type::Showdown;
        terminal_util[num_children++] = util;
    }

    void add_fold(float util) {
        assert(util > 0.0);
        child_states[num_children] = Type::Fold;
        terminal_util[num_children++] = util;
    }

    void add_child(InfoSets* child) {
        child_states[num_children] = Type::Continue;
        child->debug_position = debug_position + " " + std::to_string(num_children);

        children[num_children++] = child;
    }

    inline float get_sum_regr(int info_state_card) {
        float ret = 0.0;
        for (int child_idx = 0; child_states[child_idx] != Type::Invalid; child_idx++) {
            ret += sum_child_regr[info_state_card][child_idx];
        }
        return ret;
    }

    inline float get_strat_prob(int info_state_card, int child_idx, float sum_regr) {
        return get_regr(info_state_card, child_idx) / sum_regr;
    }

    float get_util(int card1, int card2, float prob_in) {
        const int info_state_card = is_p1 ? card1 : card2;

        // strategy is just normalized sum_child_regrs (for this state)
        float sum_regr = get_sum_regr(info_state_card);

        // util is a weighted avg of child utilities
        float util = 0.0;
        std::array<float, MAX_CHILD_STATES> child_utils;

        dprint("==%s has probs== sumregr = %f\n", debug_position.c_str(), sum_regr);
        for (int child_idx = 0; child_states[child_idx] != Type::Invalid; child_idx++) {
            dprint("%d has regr %f, ", child_idx, get_regr(info_state_card, child_idx));
            dprint("prob %f\n", get_strat_prob(info_state_card, child_idx, sum_regr));
        }
        dprint("==done with probs==\n");

        for (int child_idx = 0; child_states[child_idx] != Type::Invalid; child_idx++) {
            const float strat_prob = get_strat_prob(info_state_card, child_idx, sum_regr);

            switch (child_states[child_idx]) {
            case Type::Continue:
                child_utils[child_idx] = children[child_idx]->get_util(card1, card2, prob_in * strat_prob);
                break;
            case Type::Fold:
                child_utils[child_idx] = terminal_util[child_idx] * (is_p1 ? -1.0 : 1.0);
                break;
            case Type::Showdown:
                child_utils[child_idx] = terminal_util[child_idx] * (p1_wins(card1, card2) ? 1.0 : -1.0);
                break;
            }

            dprint("state: %s child %d has prob %f, util %f\n", debug_position.c_str(), child_idx, strat_prob, child_utils[child_idx]);

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
        for (int child_idx = 0; child_idx < num_children; child_idx++) {
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

    // returns the preferable utility for the player
    static inline float better(bool for_p1, float util1, float util2) {
        if (for_p1)
            return std::max(util1, util2);
        else
            return std::min(util1, util2);
    }

    // returns the swing from card facing card_distro in showdown
    // negative means card is more likely to lose than win
    static float get_showdown_swing(int card, const CardDistro& card_distro) {
        float sum_prob = 0.0, norm = std::accumulate(card_distro.begin(), card_distro.end(), 0.0);

        for (int p2_card = 0; p2_card < NUM_HANDS; p2_card++) {
            if (p2_card == card) continue;

            if (p1_wins(card, p2_card))
                sum_prob += card_distro[p2_card];
            else
                sum_prob -= card_distro[p2_card];
        }

        return sum_prob / norm;
    }

    // get the utility for a player facing a maximally exploitative strategy
    // the player must play their strategy
    // but the other player will always put 100% into the highest utility node for them,
    // based on the distribution of hands the opponent may have in a situation
    float get_mes_util(bool against_p1, int card, const CardDistro& card_distro) {
        // must play strategy
        if (is_p1 == against_p1) {
            float util = 0.0;
            for (int child_idx = 0; child_idx < num_children; child_idx++) {

                // the distribution that will be forwarded
                CardDistro fwd_distro = card_distro;
                // the overall probability of being in this distribution vs another child distro
                float distro_prob = 0.0;
                // for each card, there's a different filtering of the distro
                for (int fwd_card = 0; fwd_card < NUM_HANDS; fwd_card++) {
                    if (fwd_card == card)
                        continue;

                    // strat prob is the prob this fwd_card goes to this state
                    // this will reshape the card distribution
                    const float sum_regr = get_sum_regr(fwd_card);
                    const float strat_prob = get_strat_prob(fwd_card, child_idx, sum_regr);
                    fwd_distro[fwd_card] *= strat_prob;

                    // keep track of the overall probability of a distribution
                    // the sum of probs for each card across its child states must be 1
                    // so normalize this by num of hands minus 1 (-1 to account for card)
                    distro_prob += strat_prob / (NUM_HANDS - 1);
                    // this is probably a roundabout way to compute this, coming directly
                    // from regret is probably more efficient. But a) I don't have much time
                    // so TODO, and b) this is not the hot path.
                }

                const float showdown_swing = get_showdown_swing(card, fwd_distro);

                switch (child_states[child_idx]) {
                case Type::Continue:
                    util += distro_prob * children[child_idx]->get_mes_util(against_p1, card, fwd_distro);
                    break;
                case Type::Fold:
                    util += distro_prob * terminal_util[child_idx] * (is_p1 ? -1.0 : 1.0);
                    break;
                case Type::Showdown:
                    util += distro_prob * terminal_util[child_idx] * showdown_swing;
                    break;
                }
            }
            return util;
        }
        // maximally exploit case
        else {
            // here we can choose which child state to go to
            // so always pick the child state with the highest exploited util
            float util = better(is_p1, std::numeric_limits<float>::min(), std::numeric_limits<float>::max());

            float showdown_swing = get_showdown_swing(card, card_distro);

            for (int child_idx = 0; child_idx < num_children; child_idx++) {
                switch (child_states[child_idx]) {
                case Type::Continue:
                    util = better(is_p1, util, children[child_idx]->get_mes_util(against_p1, card, card_distro));
                    break;
                case Type::Fold:
                    util = better(is_p1, util, terminal_util[child_idx] * (is_p1 ? -1.0 : 1.0));
                    break;
                case Type::Showdown:
                    util = better(is_p1, util, terminal_util[child_idx] * showdown_swing);
                    break;
                }
            }

            return util;
        }
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
            cout << "Facing" << history << ": \n";

        for (int card = 0; card < NUM_HANDS; card++) {
            cout << "\tWith " << card << ":\n";

            // strategy is just normalized sum_child_regrs (for this state)
            float sum_regr = 0.0;
            for (int child_idx = 0; child_idx < num_children; child_idx++) {
                sum_regr += get_regr(card, child_idx);
            }

            for (int child_idx = 0; child_idx < num_children; child_idx++) {
                const float pct = (get_regr(card, child_idx) / sum_regr) * 100.0;

                switch (child_states[child_idx]) {
                case Type::Fold:
                    cout << "\t\tFold " << pct << "% of the time\n";
                    break;
                case Type::Showdown:
                    cout << "\t\tCall " << pct << "% of the time\n";
                    break;
                case Type::Continue:
                    // special case for checking
                    if (child_idx == 0 && history == "")
                        cout << "\t\tCheck " << child_idx << " " << pct << "% of the time\n";
                    else
                        cout << "\t\tBet sz " << child_idx << " " << pct << "% of the time\n";
                    break;
                }
            }
        }

        cout << "\n";


        for (int child_idx = 0; child_idx < num_children; child_idx++) {
            if (child_states[child_idx] != Type::Continue) continue;

            std::string next_history;

            // special case history for a check
            if (child_idx == 0 && history == "")
                next_history = history + " check";
            else
                next_history = history + " bet " + std::to_string(child_idx);

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
        InfoSets* ret = new InfoSets(p1_to_act);

        const float mip = p1_mip + p2_mip + ante;

        // terminal state: fold
        const float fold_util = p1_to_act ? p1_mip + ante : p2_mip + ante;
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

        static const Bet empty_bets;
        const Bet& this_bets = rem_bets.size() ? rem_bets.front() : empty_bets;

        // this is quite inefficient, but this is the cold path anyways
        auto next_round_rem_bets = rem_bets;

        // no more bets, only terminal states
        if (next_round_rem_bets.empty())
            return ret;

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
        InfoSets* ret = new InfoSets(false);

        // terminal state: check-through
        ret->add_showdown(ante);

        const Bet& this_bets = all_bets.front();

        auto rem_bets = all_bets;

        // no non-terminal child states
        if (rem_bets.empty())
            return ret;

        // this is quite inefficient, but this is the cold path anyways
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
        InfoSets* ret = new InfoSets(true);

        // no immediate terminal states to create

        const Bet& this_bets = all_bets.front();

        auto rem_bets = all_bets;

        // check case
        // ret->add_child(make_child_no_fold(all_bets));
        ret->add_showdown(ante);

        // no bets to consider
        if (rem_bets.empty())
            return ret;

        // this is quite inefficient, but this is the cold path anyways
        rem_bets.pop_front();

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

        return root->get_util(card1, card2, prob);
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

    // get the nash distance in this state
    std::pair<float, float> nash_dist() {
        float loss1 = 0.0, loss2 = 0.0;

        for (int card1 = 0; card1 < NUM_HANDS; card1++) {
            const float prob = starting_prob1[card1];
            loss1 += root->get_mes_util(true, card1, starting_prob2);
        }

        for (int card2 = 0; card2 < NUM_HANDS; card2++) {
            const float prob = starting_prob2[card2];
            loss2 += root->get_mes_util(false, card2, starting_prob1);
        }

        return { loss1, loss2 };
    }

    void print() {
        root->print_strat();
    }
};
