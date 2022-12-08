
#pragma once

#include "Node.h"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

class ContinuationNode: public Node {

    // all info needed at each node for CFR
    // sum_strats is the sum of all played strategies so far
    // sum_regr is the current sum of regret
    // Conecptual note: sum_regr should oscillate quite a bit between absolute strategies
    // however, sum_strats should converge to a ratio as sum_regr will oscillate in a 
    // predictable manner averaging to a Nash Equilibrium
    struct CFRInfo {
        std::array<float, NUM_HANDS> sum_strats, sum_regr;

        CFRInfo() {
            sum_regr.fill(0.0);
            sum_strats.fill(0.0);
        }
    };

    using Strategy = std::vector<float>;

    // child state -> Node
    std::vector<Node*> children;

    std::vector<CFRInfo> child_cfr_infos;

    // get the util if we are being exploited
    // distro must be normalized
    float get_exploited_util(int opp_card, const CardDistro& distro);

    // get the util if we are exploiting the opponent
    // distro must be normalized
    float get_exploiting_util(int our_card, const CardDistro& distro);

public:

    // add a child state for this node
    // I understand there's probably a more explict way to represent
    // the transfer of ownership here, but as this is a prototype and
    // the final version shouldn't use pointers at all, I don't think
    // this is worth addressing at the time being
    void add_child(Node* new_child) {
        children.push_back(new_child);
        child_cfr_infos.push_back({});
    }

    // get our running strategy (used in training) for when we're in this state and have card
    Strategy get_running_strategy(int card);

    // get the current equilibrium strategy
    Strategy get_equilibrium_strategy(int card);

    // returns the utility of being in this node
    virtual float get_util(int card1, int card2, float prob_in) final;

    // get the utility for a player facing a maximally exploitative strategy
    // the player must play their strategy
    // but the other player will always put 100% into the highest utility node for them,
    // based on the distribution of hands the opponent may have in a situation
    // against_p1: true if this is exploiting p1, false if p2
    // card: the card that the exploiter has
    // card_distro: the distribution of the exploited's hands
    virtual float get_mes_util(bool against_p1, int card, CardDistro card_distro) final;

    virtual void print_strat(const std::string& history = " ") final {
        using std::cout;

        if (is_p1)
            cout << "p1 facing" << history << "has strat:\n";
        else
            cout << "p2 facing" << history << "has strat:\n";


        for (int card = 0; card < NUM_HANDS; card++) {
            Strategy nash_strat = get_equilibrium_strategy(card);

            cout << "\tWith hand " << card << ":\n";

            for (int idx = 0; idx < children.size(); idx++) {
                cout << "\t\tAction " << idx << ": " << nash_strat[ idx ] * 100.0 << "% of the time\n";
            }
        }

        cout << "\n";

        for (int idx = 0; idx < children.size(); idx++) {
            children[ idx ]->print_strat(history + std::to_string(idx) + " ");
        }
    }

    // return the size of this class in bytes, including the size of descendant Nodes
    // this is even optimistic not taking into account extra space grabbed by vector
    virtual int size_bytes() final {
        int ret = sizeof(ContinuationNode) +
            (children.size() * sizeof(Node*)) +
            (child_cfr_infos.size() * sizeof(CFRInfo));

        for (Node* child : children)
            ret += child->size_bytes();

        return ret;
    }

    // return the size of this class actually needed for data, including descendants
    virtual int size_used() final {
        int ret = children.size() * NUM_HANDS * sizeof(float) * 2;

        for (Node* child : children)
            ret += child->size_used();

        return ret;
    }

    ContinuationNode(bool is_p1): Node(is_p1) {}

    virtual ~ContinuationNode() {
        for (Node* child : children)
            delete child;
    }
};
