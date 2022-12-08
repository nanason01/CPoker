

#pragma once

#include "Node.h"
#include "ContinuationNode.h"
#include "FoldNode.h"
#include "ShowdownNode.h"

#include <deque>
#include <vector>

class Game {
    float ante, stack_sz;

    CardDistro starting_p1, starting_p2;

    Node* root;

public:
    using Bet = std::vector<float>;

private:
    // assumed that a check ends the game
    // p1_mip: amount that p1 has put in the pot
    // p2_mip: amount that p2 has put in the pot
    // p1_wins: whether p1 wins in this game state
    // p1_to_act: whether p1 or p2 are acting in this state
    // rem_bets: deque of the remaining rounds of bet szs (all defined in terms of % of pot)
    Node* make_child(float p1_mip, float p2_mip, bool p1_to_act, const std::deque<Bet>& rem_bets) {
        ContinuationNode* ret = new ContinuationNode(p1_to_act);

        const float mip = p1_mip + p2_mip + ante;

        // terminal state: fold
        const float fold_util = p1_to_act ? p1_mip + ante : p2_mip + ante;
        ret->add_child(new FoldNode(p1_to_act, fold_util));

        // all-in catch case, can only call or fold and the util is clipped on a call
        if (p1_mip > stack_sz || p2_mip > stack_sz) {
            const float call_all_in_util = stack_sz + ante;
            ret->add_child(new ShowdownNode(p1_to_act, call_all_in_util));
            return ret;
        }

        // terminal state: call
        const float call_util = p1_to_act ? p2_mip + ante : p1_mip + ante;
        ret->add_child(new ShowdownNode(p1_to_act, call_util));

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
            const float next_bet = mip * this_bets[ bet_idx ];

            Node* child;

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
    Node* make_child_no_fold(const std::deque<Bet>& all_bets) {
        ContinuationNode* ret = new ContinuationNode(false);

        // terminal state: check-through
        ret->add_child(new ShowdownNode(false, ante));

        const Bet& this_bets = all_bets.front();

        auto rem_bets = all_bets;

        // no non-terminal child states
        if (rem_bets.empty())
            return ret;

        // this is quite inefficient, but this is the cold path anyways
        rem_bets.pop_front();

        // bet case(s)
        for (int bet_idx = 0; bet_idx < this_bets.size(); bet_idx++) {
            const float next_bet = ante * this_bets[ bet_idx ];
            Node* child = make_child(0.0, next_bet, true, rem_bets);
            ret->add_child(child);
        }

        return ret;
    }

    // special case: under the gun, you can't fold and a check isn't terminal
    // must be p1 to act
    Node* make_child_check_allowed(const std::deque<Bet>& all_bets, bool p2_can_respond) {
        ContinuationNode* ret = new ContinuationNode(true);

        // no immediate terminal states to create

        const Bet& this_bets = all_bets.front();

        auto rem_bets = all_bets;

        // check case
        if (p2_can_respond)
            ret->add_child(make_child_no_fold(all_bets));
        else
            ret->add_child(new ShowdownNode(true, ante));

        // no bets to consider
        if (rem_bets.empty())
            return ret;

        // this is quite inefficient, but this is the cold path anyways
        rem_bets.pop_front();

        // bet case(s)
        for (int bet_idx = 0; bet_idx < this_bets.size(); bet_idx++) {
            const float next_bet = ante * this_bets[ bet_idx ];
            Node* child = make_child(next_bet, 0.0, false, rem_bets);
            ret->add_child(child);
        }

        return ret;
    }

    void train_state(int card1, int card2, float prob) {
        assert(card1 != card2);

        root->get_util(card1, card2, prob);
    }

public:

    Game(
        CardDistro _starting_p1, CardDistro _starting_p2,
        float _ante, float _stack_sz, std::deque<Bet> bet_sizes,
        bool p2_can_respond)
        : ante(_ante), stack_sz(_stack_sz),
        starting_p1(_starting_p1), starting_p2(_starting_p2) {
        root = make_child_check_allowed(bet_sizes, p2_can_respond);
    }

    void train() {
        float util = 0.0;

        for (int card1 = 0; card1 < NUM_HANDS; card1++) {
            for (int card2 = 0; card2 < NUM_HANDS; card2++) {
                if (card1 == card2) continue;

                const float prob = starting_p1[ card1 ] * starting_p2[ card2 ];

                train_state(card1, card2, prob);
            }
        }
    }

    // get the nash distance in this state
    std::pair<float, float> nash_dist() {
        float loss1 = 0.0, loss2 = 0.0;

        for (int card2 = 0; card2 < NUM_HANDS; card2++) {
            const float prob = starting_p2[ card2 ];
            loss1 += prob * root->get_mes_util(true, card2, starting_p1);
        }

        for (int card1 = 0; card1 < NUM_HANDS; card1++) {
            const float prob = starting_p1[ card1 ];
            loss2 += prob * root->get_mes_util(false, card1, starting_p2);
        }

        return { loss1, loss2 };
    }

    void print() {
        root->print_strat();
    }

    int get_size_bytes() {
        return root->size_bytes();
    }

    int get_size_used() {
        return root->size_used();
    }
};
