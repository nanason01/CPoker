//
// Parameter pack helpers for int packs and Bet packs
//

#pragma once

template <typename T>
struct get {
    template <T _first, T... others>
    static constexpr T first() {
        return _first;
    }

    template <T first, T... others>
    static constexpr T last() {
        if constexpr (sizeof...(others) == 0)
            return first;
        else
            return last<others...>();
    }

    template <int index, T first, T... others>
    static constexpr T idx() {
        static_assert(index < sizeof...(others) + 1,
            "Index out of bounds");

        if constexpr (index == 0)
            return first;
        else
            return idx<index - 1, others...>();
    }

    template <T item, T first_in_pack, T... pack>
    static constexpr bool _contains_not_last() {
        if constexpr (item == first_in_pack)
            return true;
        else if constexpr (sizeof...(pack) <= 1)
            return false;
        else
            return _contains_not_last<item, pack...>();
    }

    // helper to check if an item appears in pack except for the last item in pack
    // mostly useful for checking valid state, ie 0 or 1 can't appear in the middle of an idx
    template <T item, T... pack>
    static constexpr bool contains_not_last() {
        static_assert(sizeof...(pack) > 0, "pack is empty");
        return _contains_not_last<item, pack...>();
    }

    /// debug print: this will succeed then call a template that fails, revealing items in comp
    template <T dne>
    static void fail() {}

    template <T... items>
    static void print_param_pack() {
        fail<>();
    }
};