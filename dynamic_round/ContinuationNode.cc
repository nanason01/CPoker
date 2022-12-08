
#include "ContinuationNode.h"

#include <array>
#include <cassert>
#include <limits>
#include <vector>

// get our running strategy (used in training) for when we're in this state and have card
std::vector<float> ContinuationNode::get_running_strategy(int card) {
    Strategy ret;
    float regr_norm = 0.0;

    for (const CFRInfo& cfr_info : child_cfr_infos) {
        const float regr = std::max(cfr_info.sum_regr[ card ], 0.0f);

        ret.push_back(regr);
        regr_norm += regr;
    }

    // if there are no positive regrets, return a uniform strategy
    if (regr_norm == 0.0) {
        return Strategy(child_cfr_infos.size(), 1.0 / child_cfr_infos.size());
    }

    // otherwise, normalize and return the strategy proportional to positive regrets
    for (float& val : ret) {
        val /= regr_norm;
    }
    return ret;
}

// get the current equilibrium strategy
std::vector<float> ContinuationNode::get_equilibrium_strategy(int card) {
    Strategy ret;
    float strat_norm = 0.0;

    for (const CFRInfo& cfr_info : child_cfr_infos) {
        strat_norm += cfr_info.sum_strats[ card ];
        ret.push_back(cfr_info.sum_strats[ card ]);
    }

    // if there is no data yet, return a uniform distribution
    if (strat_norm == 0.0) {
        return Strategy(child_cfr_infos.size(), 1.0 / child_cfr_infos.size());
    }

    // otherwise, normalize
    for (float& val : ret) {
        val /= strat_norm;
    }
    return ret;
}


// returns the utility of being in this node
float ContinuationNode::get_util(int card1, int card2, float prob_in) {
    const int players_card = is_p1 ? card1 : card2;

    const Strategy curr_strat = get_running_strategy(players_card);

    // util is a weighted average of child utils
    std::vector<float> child_utils;
    float util = 0.0;
    for (int idx = 0; idx < children.size(); idx++) {
        const float child_util = children[ idx ]->get_util(card1, card2, prob_in * curr_strat[ idx ]);

        child_utils.push_back(child_util);
        util += curr_strat[ idx ] * child_util;
    }

    // update regret sums, regret is the diff in util and child util
    // also update strategy sums, which just tracks the sum of all
    // strategies used so far
    for (int idx = 0; idx < children.size(); idx++) {
        const float regr = (child_utils[ idx ] - util) * (is_p1 ? 1.0 : -1.0);

        child_cfr_infos[ idx ].sum_regr[ players_card ] += prob_in * regr;
        child_cfr_infos[ idx ].sum_strats[ players_card ] += prob_in * curr_strat[ idx ];
    }

    return util;
}

// get the util if we are being exploited
// distro must be normalized
float ContinuationNode::get_exploited_util(int opp_card, const CardDistro& distro) {
    float util = 0.0;

    for (int idx = 0; idx < children.size(); idx++) {
        // compute the distro we will forward to this state
        CardDistro fwd_distro = distro;

        // also track the overall prob that we choose this child
        float child_prob = 0.0;

        for (int fwd_card = 0; fwd_card < NUM_HANDS; fwd_card++) {
            if (fwd_card == opp_card) continue;

            // this could definitely be optimized, but it doesn't need to be
            const Strategy fwd_strat = get_equilibrium_strategy(fwd_card);
            fwd_distro[ fwd_card ] *= fwd_strat[ idx ];

            child_prob += fwd_distro[ fwd_card ];
        }

        util += child_prob * children[ idx ]->get_mes_util(is_p1, opp_card, fwd_distro);
    }

    return util;
}

static inline float better(bool for_p1, float util_a, float util_b) {
    if (for_p1)
        return std::max(util_a, util_b);
    else
        return std::min(util_a, util_b);
}

// get the util if we are exploiting the opponent
// distro must be normalized
float ContinuationNode::get_exploiting_util(int our_card, const CardDistro& distro) {
    float util = better(!is_p1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());

    for (int idx = 0; idx < children.size(); idx++) {
        const float child_util = children[ idx ]->get_mes_util(!is_p1, our_card, distro);
        util = better(is_p1, util, child_util);
    }

    return util;
}

// get the utility for a player facing a maximally exploitative strategy
// the player must play their strategy
// but the other player will always put 100% into the highest utility node for them,
// based on the distribution of hands the opponent may have in a situation
// against_p1: true if this is exploiting p1, false if p2
// card: the card that the exploiter has
// card_distro: the distribution of the exploited's hands
float ContinuationNode::get_mes_util(bool against_p1, int exploiter_card, CardDistro card_distro) {
    normalize(card_distro, exploiter_card);

    // exploited case: player must play strategy
    if (is_p1 == against_p1) {
        return get_exploited_util(exploiter_card, card_distro);
    }
    // exploiter case: pick the best child state and play it with prob=1.0
    else {
        return get_exploiting_util(exploiter_card, card_distro);
    }
}
