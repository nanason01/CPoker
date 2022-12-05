//
// Compile-time define available bet sizings
//

#pragma once

#include <ratio>

#include "Pack.h"

// note: floating point template arguments are not allowed, 
// these numbers are % of pots
template <int... szs>
struct Bet {
    static_assert(sizeof...(szs) > 0, "Invalid bet: bet must have at least 1 available size");

    static constexpr size_t count() { return sizeof...(szs); }

    template <int idx>
    static constexpr auto idx() {
        static_assert(idx > 1, "Fold or check not in bet");

        // -2 comes from the offset from: fold = 0, check = 1, bet1 = 2...
        return std::ratio<get<int>::idx<idx - 2, szs...>(), 100>();
    }
};