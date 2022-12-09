

#pragma once

#include "Node.h"

#include <cassert>
#include <array>

class FoldNode: public Node {
public:

    float util;

    // the util of being in a fold state is just the money in pot times the amount folded
    // nothing to train here
    virtual float get_util(int card1, int card2, float prob_in) final {
        return util;
    }

    // same as above, util of folding doesn't depend on any inputs
    virtual float get_mes_util(bool against_p1, int card, CardDistro card_distro) final {
        return util;
    }

    // prints the strat, nothing to do for a terminal node
    virtual void print_strat(const std::string& history = " ") final {}

    // return the size of this class in bytes, including the size of descendant Nodes
    virtual int size_bytes() final {
        return sizeof(FoldNode);
    }

    // return the size of this class actually needed for data, including descendants
    virtual int size_used() final {
        // these are returning a util that can be compile-time calculated
        return 0;
    }

    FoldNode(bool _is_p1, float _util): Node(_is_p1), util(_util* (_is_p1 ? -1.0 : 1.0)) {
        assert(_util > 0.0);
    }
};
