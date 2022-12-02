//
// The goal of this is to test if there is a significant perf
// improvement from statically traversing the game tree vs dynamically
//
// This still makes the assumption that there is a simple total ordering
// of hands known up-front
//
// There are an arbitrary number of rounds, defined in the
// ctor of the dynamic version and template arguments in the static
//
// To make things more complicated, this assumes that there are
// hands 1-20, and that ANTE is put in the pot to start
//

#include <vector>
#include <array>
#include <tuple>

// STACK_SIZE is independant from ANTE, ie STACK_SIZE is the remaining chips that can go in play
constexpr float ANTE = 1.0;
constexpr float STACK_SIZE = 10.0;

using std::vector;
using std::array;
using std::tuple;

// parameter pack helpers for int packs and Bet packs

template <typename T>
struct get {
    template <T first, T... others>
    static constexpr T first() {
        return first;
    }

    template <T... others, T last>
    static constexpr T last() {
        return last;
    }

    template <int index, T first, T... others>
    static constexpr T idx() {
        if constexpr (index == 0)
            return first;
        else
            return idx<index - 1, others...>();
    }
};


// note: floating point template arguments are not allowed, 
// these numbers are % of pots
template <int... szs>
struct Bet {
    static_assert(sizeof...(szs) > 0, "Empty betting rounds don't make any sense");

    static constexpr size_t count() { return sizeof...(szs); }

    template <int idx>
    static constexpr float idx() {
        static_assert(idx > 1, "Fold or check not in bet");

        // -2 comes from the offset from: fold = 0, check = 1, bet1 = 2...
        return get<int>::idx<idx - 2, szs...>() / 100.0;
    }
};

//
// Conceptually, each bet is recursive given some input "P of reaching this state"
// p1 is first to act, p2 is second to act
//
// The bets are defined symetrically, p1 can bet any of bets[0] amount, then
// p2 can fold, call, or raise any of bets[1] amount, then p1 F, C, or R bets[2]...
// if p1 checks initially, then p2 can bet any of bets[0] and the round will play
// with mirrored options. If this is a bad assumption, this can be revisited, but
// this is mostly assuming that symettrical options are representative enough,
// such that I'd be defining everything twice in the calling functions
//
// The definition of the index of a state in the tree:
// the index of a "state" is the series of choices made to get somewhere
// at each point, folding will always be idx 0 (sometimes invalid)
// calling/checking always idx 1
// and any available bet/raise sizes defined indexed from 2 on
//
template <size_t nr_hands, typename... bets>
class Round {
    static_assert(sizeof...(bets) > 0, "Must have at least one betting round");

    template <int bet_idx, typename _first_bet, typename... _bets>
    static constexpr auto _get_bet() {
        if constexpr (bet_idx == 0)
            return _first_bet();
        else
            return _get_bet<bet_idx - 1, bets...>();
    }

    template <int bet_idx>
    static constexpr auto get_bet() {
        return _get_bet<bet_idx, bets...>();
    }

    // the initial probability for each hand per player
    array<float, nr_hands> p1_starting, p2_starting;

    // a mapping from hand -> int, where f(a) > f(b) iff hand a beats hand b
    array<int, nr_hands> hand_rankings;

    // check whether this idxs pack represents a valid state
    // doesn't check any out of boundness, that will be checked elsewhere
    template <int... idxs>
    static constexpr inline void check_valid_idx() {
        static_assert(get<int>::first<idxs...>() != 0, "Invalid state: Can't fold first to act");

        static_assert(sizeof...(idxs) > 1 &&
            get<int>::first<idxs...>() == 1 &&
            get<int>::idx<1, idxs...>() == 0,
            "Invalid state: Can't fold facing a check");

        // TODO: check for invalid action after a terminal state
    }

    // helper to recursively traverse bets
    template <int... idxs, int last>
    static constexpr inline float _mta_helper() {
        constexpr auto curr_bet = get_bet<sizeof...(idxs)>();

        if constexpr (sizeof...(idxs) == 0)
            return ANTE * curr_bet. template idx<last>();
        else
            return (1 + curr_bet) * _mta_helper<idxs...>();
    }

    // returns utility required to do the last action
    // it may make sense to support terminal states: ie those ending in check/fold
    template <int... idxs>
    static constexpr inline float _mta() {
        static_assert(sizeof...(idxs) > 0, "Invalid call to _mta with empty state");
        static_assert(get<int>::last<idxs...>() > 1, "Invalid call to _mta with ending check/fold");

        if (_mta_helper<idxs...>() >= STACK_SIZE)
            return STACK_SIZE;
        else
            return _mta_helper<idxs...>();
    }

    // mta: Money To Act
    // returns the utility required to do the last action
    // eg bet 0.7x then raise 3x will return (0.7*ANTE)*3.0
    // currently no use case where the last action was fold/check, so that isn't allowed
    template <int first, int... idxs>
    static constexpr inline float mta() {
        check_valid_idx<first, idxs...>();

        // first action being a check means shift back
        if constexpr (first == 1) {
            if constexpr (sizeof...(idxs) == 0)
                return 0.0;
            else
                return _mta<idxs...>();
        } else {
            return _mta<first, idxs...>();
        }
    }
    template <>
    static constexpr inline float mta() {
        return 0.0;  // initial state case
    }

    // mip: Money In Pot
    // returns the utility in some game tree state
    // allows for terminal states, fold is disregarded and a call
    // just matches the prev bet (as it should)
    template <int... idxs, int prev, int last>
    static constexpr inline float mip() {
        static_assert(prev > 0, "Action after a fold");

        if constexpr (last == 0) {  // last act fold case
            return mta<idxs...>() + mta<idxs..., prev>();
        } else if constexpr (last == 1) {  // last act call case
            return 2.0 * mta<idxs..., prev>();
        } else {  // last act bet/raise case
            return mta<idxs..., prev>() + mta<idxs..., prev, last>();
        }
    }
    template <int first>
    static constexpr inline float mip() {
        if constexpr (first == 1)
            return 0.0;  // check case
        else
            return mta<first>();  // single bet case
    }
    template <>
    static constexpr inline float mip() {
        return 0.0;  // no action case
    }

    template <int... idxs>
    static constexpr inline bool is_all_in() {
        return mta<idxs...>() == STACK_SIZE;
    }

    template <int bet_num, int... idxs>
    static constexpr inline int _count_num_bets() {
        constexpr int num_bets_possible = decltype(get_bet<bet_num>())::count();
        if constexpr (bet_num == num_bets_possible - 1)
            return num_bets_possible;

        // if this bet size is all_in, all larger bets are also all-ins,
        // thus, we're done counting
        // +2 offset accounts for fold/check in bet idxing
        if constexpr (is_all_in<idxs..., bet_num + 2>())
            return bet_num + 1;

        return _count_num_bets<bet_num + 1, idxs...>();
    }

    // returns the number of different bets descendant from this state
    // clips all-ins into single action
    // this doesn't make sense if there's no action after
    // this doesn't account for check/call/fold in the count, just bets
    template <int... idxs>
    static constexpr inline int count_num_bets() {
        static_assert(sizeof...(idxs) < sizeof...(bets), "No child states");
        static_assert(!is_all_in<idxs...>(), "No bets after all-in");

        return _count_num_bets<0, idxs...>();
    }

    // returns true if this is a terminal state
    template <int... idxs>
    static constexpr inline bool is_terminal() {
        if constexpr (sizeof...(idxs) < 2)
            return false;

        return get<int>::last<idxs...>() < 2;
    }

    // num children with assumed no intial check
    template <int... idxs>
    static constexpr inline int _num_children() {
        static_assert(!is_terminal<idxs...>(), "terminal state has no children");

        // no action yet, can't fold
        if constexpr (sizeof...(idxs) == 0)
            return 1 + count_num_bets<idxs...>();

        // no more available bets
        if constexpr (sizeof...(idxs) == sizeof...(bets))
            return 2;

        // last to act was all-in
        if constexpr (is_all_in<idxs...>())
            return 2;

        // normal case, num of valid bets next
        return 2 + count_num_bets<idxs...>();
    }

    // return the number of children from this state
    template <int first, int... idxs>
    static constexpr inline int num_children() {
        check_valid_idx<first, idxs...>();

        if constexpr (first == 1)
            return _num_children<idxs...>();
        else
            return _num_children<first, idxs...>();
    }
    template <>
    static constexpr inline int num_children() {
        return _num_children<>();
    }
};

/// below here is testing/sanity checking

#include <iostream>

using std::cout;

int main() {
    using bet = Bet<50, 75, 110>;

    cout << bet::count() << "\n";
    cout << bet::idx<2>() << " " << bet::idx<3>() << " " << bet::idx<4>() << "\n";

    Round<10, Bet<50, 75, 100, 125>, Bet<300, 400, 500>> main_round;


}