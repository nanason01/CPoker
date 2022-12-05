//
// This is the templated struct to hold actual data for each node
// There must be a float for each child state to represent the cumulative
// regret for that state
//
// Terminal states have no children, thus there are no regret values to store for them
//

#pragma once

#include "Configs.h"
#include "Pack.h"
#include "Round.h"

template <typename first_round, typename... rounds>
struct _Node {
    typename <int util_in_node, typename first_idx_pack, typename... idx_packs>
        struct _Node_internal {

    };
};

template <typename... rounds>
struct Node {

};