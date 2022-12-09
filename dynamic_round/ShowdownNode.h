
#pragma once

#include "Node.h"

#include <cassert>
#include <array>

class ShowdownNode: public Node {
public:

    float util;

    // the util of being in a fold state is just the money in pot times the amount folded
    // nothing to train here
    virtual float get_util(int card1, int card2, float prob_in) final {
        if (p1_wins(card1, card2))
            return util;
        else
            return -1.0 * util;
    }

    // same as above, util of folding doesn't depend on any inputs
    // if against_p1, then card_distro is p1's and card is p2's
    // otherwise it's the opposite
    // TODO this relies on ties not being accounted for
    virtual float get_mes_util(bool against_p1, int card, CardDistro card_distro) final {
        normalize(card_distro);

        float swing = 0.0;
        for (int other_card = 0; other_card < NUM_HANDS; other_card++) {
            if (other_card == card) continue;

            if (p1_wins(other_card, card))
                swing += card_distro[ other_card ];
            else
                swing -= card_distro[ other_card ];
        }

        return util * swing * (against_p1 ? 1.0 : -1.0);
    }

    // prints the strat, nothing to do for a terminal node
    virtual void print_strat(const std::string& history = "") final {}

    // return the size of this class in bytes, including the size of descendant Nodes
    virtual int size_bytes() final {
        return sizeof(ShowdownNode);
    }

    // return the size of this class actually needed for data, including descendants
    virtual int size_used() final {
        // these are returning a util that can be compile-time calculated
        return 0;
    }

    ShowdownNode(bool _is_p1, float _util): Node(_is_p1), util(_util) {
        assert(_util > 0.0);
    }
};
