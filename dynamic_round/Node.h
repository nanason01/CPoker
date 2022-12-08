
#pragma once

#include <array>
#include <vector>
#include <string>
#include <cassert>
#include <numeric>

constexpr int NUM_HANDS = 10;

using CardDistro = std::array<float, NUM_HANDS>;

class Node {
public:

    bool is_p1;

    // normalize this CardDistro
    // if discard_card is set, that card will be prob 0.0
    static void normalize(CardDistro& distro, int discard_card = -1) {
        if (discard_card != -1)
            distro[ discard_card ] = 0.0;
        const float norm = std::accumulate(distro.begin(), distro.end(), 0.0);

        if (norm == 0.0) {
            distro.fill(1.0 / (discard_card == -1 ? NUM_HANDS : NUM_HANDS - 1));
            if (discard_card != -1)
                distro[ discard_card ] = 0.0;
            return;
        }

        for (float& val : distro)
            val /= norm;
    }

    // return who wins in this situation
    // currently does not account for ties
    static bool p1_wins(int card1, int card2) {
        return card1 > card2;
    }

    // returns the utility of being in this node
    virtual float get_util(int card1, int card2, float prob_in) = 0;

    // get the utility for a player facing a maximally exploitative strategy
    // the player must play their strategy
    // but the other player will always put 100% into the highest utility node for them,
    // based on the distribution of hands the opponent may have in a situation
    // against_p1: true if this is exploiting p1, false if p2
    // card: the card that the exploiter has
    // card_distro: the distribution of the exploited's hands
    virtual float get_mes_util(bool against_p1, int card, CardDistro card_distro) = 0;

    virtual void print_strat(const std::string& history = " ") = 0;

    // return the size of this class in bytes, including the size of descendant Nodes
    virtual int size_bytes() = 0;
    // return the size of this class actually needed for data, including descendants
    virtual int size_used() = 0;

    Node(bool _is_p1): is_p1(_is_p1) {}
    virtual ~Node() = default;
};
