//
// The main driver for a game, this handles the computing utility and storing a strategy
// Heavily depends on Round and Bet to template out the structure of the game
// as well as the helper functions therein, but this is the only class of the
// 3 that actually has data
//



#pragma once

#include <array>

template <int nr_hands, typename ante, typename... rounds>
class Game {

    template <int round_num, typename first_round, typename... others>
    static constexpr auto _get_round() {
        if constexpr (round_num == 0)
            return first_round{};
        else
            return _get_round<round_num - 1, others...>();
    }

    template <int round_num>
    static constexpr auto get_round() {
        return _get_round<round_num, rounds...>();
    }

    // todo: add more bools
    // is continuation state: specialization for beginning of another round
    // has child: specialization if it has a child to consider
    // has sibling: "" but for a sibling
    // (the last two avoid instantiating an empty Node; these can replace valid)


    // functions TODO
    // bool Round<idxs>::is_valid()
    // bool Round<idxs>::is_continuation() -> true if this ends a round, non-terminally (incl showdown)
    // bool Round<idxs>::is_terminal() -> true if this state ends the game, ie folded (excl showdown)
    // int Round<idxs>::start_of_next() -> returns the lowest child idx state (1 if check, 0 otherwise)
    //      (will be called (but not used) on terminal states, doesn't need to be too compicated)

    template <int... starting_cards>
    struct Root {
        using first_round = decltype(get_round<0>());

        static constexpr bool root_has_child = first_round::is_valid<1>();
        static constexpr bool root_has_sibling = first_round::is_valid<2>();

        template<int card>
        static constexpr bool is_starting_card() {
            return get<int>::contains<card, starting_cards...>();
        }

        Node<
            Root<starting_cards...>,
            false,
            root_has_child,
            root_has_sibling,
            0,
            ANTE> root;
    };

    template <typename parent, bool is_continuation, bool has_child, bool has_sibling, int round_num, typename curr_ante, int... idxs>
    struct Node {
        static_assert(!sizeof(Node), "Invalid template instantiation, impossible state");
    };

    // continuation Node: represents the end of a round, transitions to the next round
    // siblings are other continuation nodes (ie other cards were drawn)
    // children are the beginning of the next round
    // has_child in this context would be the next round
    template <typename parent, int round_num, typename curr_ante, int... idxs>
    struct Node<parent, true, true, true, curr_ante, idxs...> {
        static constexpr bool is_continuation = true;

        template<int card>
        static constexpr bool is_starting_card() {
            return parent::is_starting_card<card>();
        }

    };

    // continuation Node: represents the end of a round, transitions to the next round
    // children are the beginning of the next round
    // this one has no siblings
    template <int round_num, typename curr_ante, int... idxs>
    struct Node<true, true, false, curr_ante, idxs...> {

    };

};