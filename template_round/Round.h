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
// hands 1-20, and that ante is put in the pot to start
//

#pragma once

#include <ratio>

#include "Pack.h"
#include "Bet.h"
#include "Configs.h"  // TODO: eventually refactor this out


// this is a TODO
// eventually these will be passed to Game, and Game will arrange the Rounds
// assigning the proper input ante to each one
template <typename... bets>
class Round {
public:
    _Round<ANTE, bets...> internal;
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
template <typename ante, typename... bets>
class _Round {
public:  // TODO: finegrain public/private fxns
    static_assert(sizeof...(bets) > 0, "Must have at least one betting round");

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

    template <int first_idx, int... idxs>
    static constexpr void _check_valid_idx_nonempty() {
        static_assert(first_idx != 0, "Invalid state: Can't fold first to act");

        if constexpr (first_idx == 1) {
            if constexpr (sizeof...(idxs) > 0)
                _check_valid_idx_nonempty<idxs...>();
        } else {
            _check_valid_idx<first_idx, idxs...>();
        }
    }

    // check whether this idxs pack represents a valid state
    // doesn't check any out of boundness, that will be checked elsewhere
    template <int... idxs>
    static constexpr void check_valid_idx() {
        if constexpr (sizeof...(idxs) > 0)
            _check_valid_idx_nonempty<idxs...>();
        // empty idx is valid
    }

    template <int bet_idx, int bet_num, typename _first_bet, typename... _bets>
    static constexpr auto _get_bet() {
        if constexpr (bet_num == 1)
            return _first_bet::template idx<bet_idx>();
        else
            return _get_bet<bet_idx, bet_num - 1, _bets...>();
    }

    // returns the bet size at this idxs pack, not accounting for all_in
    template <int first_idx, int... idxs>
    static constexpr auto get_bet() {
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
    struct _mta_idx {
        template <int num, int... partial_idxs>
        static constexpr auto _get() {
            if constexpr (sizeof...(partial_idxs) < num) {
                constexpr int next_idx = get<int>::template idx<sizeof...(partial_idxs), idxs...>();
                return _get<num, partial_idxs..., next_idx>();
            } else {
                return mta<partial_idxs...>();
            }
        }
    };

    // mta: Money To Act
    // returns the utility required to do the last action
    // only considers the first num idxs
    template <int num, int... idxs>
    static constexpr auto mta_idx() {
        return _mta_idx<idxs...>::template _get<num>();
    }

    template <int... idxs>
    struct _mip_idx {
        template <int num, int... partial_idxs>
        static constexpr auto _get() {
            if constexpr (sizeof...(partial_idxs) < num) {
                constexpr int next_idx = get<int>::template idx<sizeof...(partial_idxs), idxs...>();
                return _get<num, partial_idxs..., next_idx>();
            } else {
                return mip<partial_idxs...>();
            }
        }
    };

    // mip: Money In Pot
    // returns the total utility in the pot in some situation
    // only considers the first num idxs
    template <int num, int... idxs>
    static constexpr auto mip_idx() {
        return _mip_idx<idxs...>::template _get<num>();
    }

    template <int... idxs>
    static constexpr auto _mta_no_check() {
        if constexpr (sizeof...(idxs) == 0) {
            return 0.0;
        } else {
            constexpr auto amt = std::ratio_multiply<
                decltype(get_bet<idxs...>()),
                decltype(mip_idx<sizeof...(idxs) - 1, idxs...>())>{};

            if constexpr (std::ratio_greater_equal<decltype(amt), STACK_SIZE>::value)
                return STACK_SIZE{};
            else
                return amt;
        }
    }

    template <int check, int... idxs>
    static constexpr auto _mta_rm_first() {
        return _mta_no_check<idxs...>();
    }

    // mta: Money To Act
    // returns the utility required to do the last action
    // eg bet 0.7x then raise 3x will return (0.7*ante)*3.0
    // currently no use case where the last action was fold/check, so that isn't allowed
    template <int... idxs>
    static constexpr auto mta() {
        if constexpr (sizeof...(idxs) == 0)
            return 0.0;
        else if constexpr (get<int>::first<idxs...>() == 1)
            return _mta_rm_first<idxs...>();
        else
            return _mta_no_check<idxs...>();
    }

    // all mip helpers assume that the idx has been normalized
    // ie no checks
    // get money in pot when the last action was a fold
    template <int... idxs>
    static constexpr auto _mip_fold() {
        static_assert(get<int>::last<idxs...>() == 0);
        static_assert(sizeof...(idxs) >= 2);

        constexpr int prev1 = sizeof...(idxs) - 1;
        constexpr int prev2 = sizeof...(idxs) - 2;

        if constexpr (prev2 > 0) {
            return std::ratio_add<
                ante, std::ratio_add<
                decltype(mta_idx<prev2, idxs...>()),
                decltype(mta_idx<prev1, idxs...>())>
            >{};
        } else {
            return std::ratio_add<ante,
                decltype(mta_idx<prev1, idxs...>())>{};
        }
        // can't fold facing nothing, no other (valid) case
    }

    // get money in pot when the last action was a call
    template <int... idxs>
    static constexpr auto _mip_call() {
        static_assert(get<int>::last<idxs...>() == 1);

        constexpr int prev1 = sizeof...(idxs) - 1;

        if constexpr (prev1 > 0) {
            return std::ratio_add<ante, std::ratio_add<
                decltype(mta_idx<prev1, idxs...>()),
                decltype(mta_idx<prev1, idxs...>())>
            >{};
        } else {
            // check-through
            return ante{};
        }
    }

    // get money in pot when the last action was a bet
    template <int... idxs>
    static constexpr auto _mip_bet() {
        static_assert(get<int>::last<idxs...>() >= 2);
        static_assert(sizeof...(idxs) > 0);

        constexpr int curr = sizeof...(idxs);
        constexpr int prev1 = sizeof...(idxs) - 1;

        if constexpr (prev1 > 0) {
            return std::ratio_add<ante, std::ratio_add<
                decltype(mta_idx<prev1, idxs...>()),
                decltype(mta_idx<curr, idxs...>())>
            >{};
        } else {
            return std::ratio_add<ante, decltype(mta_idx<curr, idxs...>())>{};
        }
    }

    // mip without an initial check
    template <int... idxs>
    static constexpr auto _mip() {
        if constexpr (sizeof...(idxs) == 0)
            return ante{};
        else if constexpr (get<int>::last<idxs...>() == 0)
            return _mip_fold<idxs...>();
        else if constexpr (get<int>::last<idxs...>() == 1)
            return _mip_call<idxs...>();
        else
            return _mip_bet<idxs...>();
    }

    template <int first, int... idxs>
    static constexpr auto _mip_rm_first() {
        return _mip<idxs...>();
    }

    // mip: Money In Pot
    // returns the utility in some game tree state
    // allows for terminal states, fold is disregarded and a call
    // just matches the prev bet (as it should)
    template <int... idxs>
    static constexpr auto mip() {
        check_valid_idx<idxs...>();

        if constexpr (sizeof...(idxs) == 0)
            return ante{};
        else if constexpr (get<int>::first<idxs...>() == 1)
            return _mip_rm_first<idxs...>();
        else
            return _mip<idxs...>();
    }

    template <int... idxs>
    static constexpr bool is_all_in() {
        return std::ratio_greater_equal<
            decltype(mta<idxs...>()),
            STACK_SIZE>::value;
    }

    template <int bet_num, typename _first_bet, typename... _bets>
    static constexpr size_t _get_num_bets_possible() {
        static_assert(bet_num > 0);

        if constexpr (bet_num == 1)
            return _first_bet::count();
        else
            return _get_num_bets_possible<bet_num - 1, _bets...>();
    }

    template <int bet_num, int... idxs>
    static constexpr int _count_num_bets() {
        static_assert(sizeof...(idxs) < sizeof...(bets), "No child bets defined");

        constexpr int num_bets_possible = _get_num_bets_possible<sizeof...(idxs) + 1, bets...>();
        if constexpr (bet_num == num_bets_possible - 1)
            return num_bets_possible;
        // if this bet size is all_in, all larger bets are also all-ins,
        // thus, we're done counting
        // +2 offset accounts for fold/check in bet idxing
        else if constexpr (is_all_in<idxs..., bet_num + 2>())
            return bet_num + 1;
        else
            return _count_num_bets<bet_num + 1, idxs...>();
    }

    template <int first, int... idxs>
    static constexpr int _count_num_bets_rm_first() {
        return _count_num_bets<0, idxs...>();
    }

    // returns the number of different bets descendant from this state
    // clips all-ins into single action
    // this doesn't make sense if there's no action after
    // this doesn't account for check/call/fold in the count, just bets
    template <int... idxs>
    static constexpr int count_num_bets() {
        check_valid_idx<idxs...>();
        static_assert(!is_all_in<idxs...>(), "No bets after all-in");

        if constexpr (sizeof...(idxs) == 0)
            return _count_num_bets<0>();
        else if constexpr (get<int>::first<idxs...>() == 1)
            return _count_num_bets_rm_first<idxs...>();
        else
            return _count_num_bets<0, idxs...>();
    }

    // returns true if this is a terminal state
    template <int... idxs>
    static constexpr bool is_terminal() {
        check_valid_idx<idxs...>();

        if constexpr (sizeof...(idxs) < 2)
            return false;
        else
            return get<int>::last<idxs...>() < 2;
    }

    // num children with assumed no intial check
    template <int... idxs>
    static constexpr int _num_children() {
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

    template <int first, int... idxs>
    static constexpr int _num_children_rm_first() {
        return _num_children<idxs...>();
    }

    // return the number of children from this state
    template <int... idxs>
    static constexpr int num_children() {
        check_valid_idx<idxs...>();
        static_assert(!is_terminal<idxs...>(), "terminal state has no children");

        if constexpr (sizeof...(idxs) == 0)
            return _num_children<>();
        else if constexpr (get<int>::first<idxs...>() == 1)
            return _num_children_rm_first<idxs...>();
        else
            return _num_children<idxs...>();
    }
};
