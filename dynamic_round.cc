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
public:  // TODO: finegrain public/private fxns
    static_assert(sizeof...(bets) > 0, "Must have at least one betting round");

    // the initial probability for each hand per player
    array<float, nr_hands> p1_starting, p2_starting;

    // a mapping from hand -> int, where f(a) > f(b) iff hand a beats hand b
    array<int, nr_hands> hand_rankings;

    // helper to check whether a state has any invalid action after terminal state
    // doesn't check for all-in terminal states, TODO, but currently that would be circular
    template <int... idxs>
    static constexpr void _check_valid_idx() {
        static_assert(sizeof...(idxs) > 0, "empty idx passed to _check_valid_idx");
        static_assert(!get<int>::contains_not_last<0, idxs...>(),
            "Invalid state: action after a fold");
        static_assert(!get<int>::contains_not_last<1, idxs...>(),
            "Invalid state: action after a terminal call");
    }

    // check whether this idxs pack represents a valid state
    // doesn't check any out of boundness, that will be checked elsewhere
    template <int first_idx, int... idxs>
    static constexpr void check_valid_idx() {
        static_assert(first_idx != 0, "Invalid state: Can't fold first to act");

        if constexpr (first_idx == 1) {
            if constexpr (sizeof...(idxs) > 0)
                check_valid_idx<idxs...>();
        } else {
            _check_valid_idx<first_idx, idxs...>();
        }
    }
    static constexpr void check_valid_idx() {
        // empty state is valid
    }

    template <int bet_idx, int bet_num, typename _first_bet, typename... _bets>
    static constexpr float _get_bet() {
        if constexpr (bet_num == 1)
            return _first_bet::template idx<bet_idx>();
        else
            return _get_bet<bet_idx, bet_num - 1, _bets...>();
    }

    // returns the bet size at this idxs pack, not accounting for all_in
    template <int first_idx, int... idxs>
    static constexpr float get_bet() {
        check_valid_idx<first_idx, idxs...>();

        if constexpr (sizeof...(idxs) == 0) {
            return _get_bet<first_idx, 1, bets...>();
        } else {
            constexpr int last_idx = get<int>::last<idxs...>();

            if constexpr (first_idx == 1)
                return _get_bet<last_idx, sizeof...(idxs), bets...>();
            else
                return _get_bet<last_idx, sizeof...(idxs) + 1, bets...>();
        }
    }

    template <int... idxs>
    struct _mta {
        // get the mta with this number of idxs included
        template <int num, int... partial_idxs>
        static constexpr float _amt() {
            // done building template case
            if constexpr (sizeof...(partial_idxs) == num) {
                constexpr float curr_bet = get_bet<partial_idxs...>();

                if constexpr (num == 0)
                    return 0.0;
                else if constexpr (num == 1)
                    return ANTE * curr_bet;
                else
                    return (1 + curr_bet) * _amt<num - 1>();
            } else {
                constexpr int next_idx = get<int>::idx<sizeof...(partial_idxs), idxs...>();
                return _amt<num, partial_idxs..., next_idx>();
            }
        }
    };

    // mta: Money To Act
    // returns the utility required to do the last action
    // eg bet 0.7x then raise 3x will return (0.7*ANTE)*3.0
    // currently no use case where the last action was fold/check, so that isn't allowed
    template <int... idxs>
    static constexpr float mta() {
        constexpr float actual_amt = _mta<idxs...>::template _amt<sizeof...(idxs)>();

        if constexpr (actual_amt >= STACK_SIZE)
            return STACK_SIZE;
        else
            return actual_amt;
    }

    // all mip helpers assume that the idx has been normalized
    // ie no checks
    // get money in pot when the last action was a fold
    template <int... idxs>
    static constexpr float _mip_fold() {
        static_assert(get<int>::last<idxs...>() == 0);
        static_assert(sizeof...(idxs) >= 2);

        constexpr int prev1 = sizeof...(idxs) - 1;
        constexpr int prev2 = sizeof...(idxs) - 2;

        if constexpr (prev2 > 0) {
            return ANTE +
                _mta<idxs...>::template _amt<prev2>() +
                _mta<idxs...>::template _amt<prev1>();
        } else {
            return ANTE +
                _mta<idxs...>::template _amt<prev1>();
        }
        // can't fold facing nothing
    }

    // get money in pot when the last action was a call
    template <int... idxs>
    static constexpr float _mip_call() {
        static_assert(get<int>::last<idxs...>() == 1);

        constexpr int prev1 = sizeof...(idxs) - 1;

        if constexpr (prev1 > 0) {
            return ANTE + 2.0 * _mta<idxs...>::template _amt<prev1>();
        } else {
            // check-through
            return ANTE;
        }
    }

    // get money in pot when the last action was a bet
    template <int... idxs>
    static constexpr float _mip_bet() {
        static_assert(get<int>::last<idxs...>() >= 2);
        static_assert(sizeof...(idxs) > 0);

        constexpr int curr = sizeof...(idxs);
        constexpr int prev1 = sizeof...(idxs) - 1;

        if constexpr (prev1 > 0) {
            return ANTE +
                _mta<idxs...>::template _amt<prev1>() +
                _mta<idxs...>::template _amt<curr>();
        } else {
            return ANTE +
                _mta<idxs...>::template _amt<curr>();
        }
    }

    // mip without an initial check
    template <int... idxs>
    static constexpr float _mip() {
        if constexpr (sizeof...(idxs) == 0)
            return ANTE;
        else if constexpr (get<int>::last<idxs...>() == 0)
            return _mip_fold<idxs...>();
        else if constexpr (get<int>::last<idxs...>() == 1)
            return _mip_call<idxs...>();
        else
            return _mip_bet<idxs...>();
    }

    template <int first, int... idxs>
    static constexpr float _mip_rm_first() {
        return _mip<idxs...>();
    }

    // mip: Money In Pot
    // returns the utility in some game tree state
    // allows for terminal states, fold is disregarded and a call
    // just matches the prev bet (as it should)
    template <int... idxs>
    static constexpr float mip() {
        check_valid_idx<idxs...>();

        if constexpr (sizeof...(idxs) == 0)
            return ANTE;
        else if constexpr (get<int>::first<idxs...>() == 1)
            return _mip_rm_first<idxs...>();
        else
            return _mip<idxs...>();
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
    static constexpr inline int num_children() {
        return _num_children<>();
    }
};

/// below here is testing/sanity checking

#include <iostream>

using std::cout;

int main() {

    using main_round = Round<10, Bet<50, 75, 100>, Bet<200, 500>>;

    cout << "get_bet: " << main_round::get_bet<4, 2>() << std::endl;
    cout << "_mta: " << main_round::_mta<4, 2>::_amt<1>() << std::endl;

    cout << "mta: " << main_round::mta<4, 2>() << std::endl;
    cout << "mip: " << main_round::mip<3>() << std::endl;
    cout << "mip: " << main_round::mip<4, 2>() << std::endl;
    cout << "mip: " << main_round::mip<3, 1>() << std::endl;

}