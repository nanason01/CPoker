//
// Just a demo of a solver on a basic 1 2 3 game
//
// Both players get 1, 2, or 3 (without duplicates)
// If it makes it to showdown, the player with higher card wins
// They have to ante 1 point each
// The first player can either bet 1 point or check
// The second player can either call or fold, or check (no betting)
//

#include <vector>
#include <iostream>

using namespace std;

// All utilities are defined in terms of player 1's gain
using Utility = float;

struct Node {
    float sum_regret;
};

class GameTree {
    const vector<float> init_probs;

    vector<Node> first_action;
    vector<Node> first_bet_response;

public:

    // game state probabilities (all possible hole cards are equally likely)
    // idx 1 is player 1: 1, player 2: 2.
    // eg idx 4 is 1: 2, 2: 3 ...

    // first action strategy (player 1 may bet or check over 3 cards)
    // idx 1 is player 1: 1, bet
    // idx 2 is player 1: 1, check ...
    // eg idx 4 is player 1: 2: check

    // second player bet response
    // in each information set, call or fold probs
    // idx 1 is player 2: 1, call
    // idx 2 is player 2: 1, fold
    // eg idx 5 is player 2: 3, call
    // ie for each card 2 has, call or fold

    // All utilities are defined for each total game state

    // 2nd player folding makes player 1 win 1.0 in any state
    GameTree()
        :
        init_probs({ 0.5, 0.0, 0.0, 0.0, 0.0, 0.5 }),
        first_action(6, { 0.01 }), first_bet_response(6, { 0.01 }) {}

    Utility get_call_utility(int card_1, int card_2) {
        return card_1 > card_2 ? 3.0 : -3.0;
    }

    Utility get_fold_utility(int card_1, int card_2) {
        return 1.0;
    }

    float get_init_prob(int card_1, int card_2) {
        switch (card_1) {
        case 1:
            if (card_2 == 2)
                return init_probs[0];
            else
                return init_probs[1];
        case 2:
            if (card_2 == 1)
                return init_probs[2];
            else
                return init_probs[3];
        case 3:
            if (card_2 == 1)
                return init_probs[4];
            else
                return init_probs[5];
        }

        return 0.0;
    }

    // Train this outcome once and return the utility
    Utility train_bet_response(int card_1, int card_2, float prob, bool print = false) {
        // get our current strategy
        float sum_call_regr = first_bet_response[(card_2 - 1) * 2].sum_regret;
        float sum_fold_regr = first_bet_response[(card_2 - 1) * 2 + 1].sum_regret;

        float sum_regr = sum_call_regr + sum_fold_regr;

        float call = sum_call_regr / sum_regr;
        float fold = sum_fold_regr / sum_regr;

        // calc utility of each action
        Utility call_util = get_call_utility(card_1, card_2);
        Utility fold_util = get_fold_utility(card_1, card_2);

        if (print)
            cout << "train bet resp with " << card_1 << " " << card_2 << "\n";

        if (print)
            cout << "call util: " << call_util << " fold util: " << fold_util << "\n";

        if (print)
            cout << "call regr: " << sum_call_regr << " fold regr: " << sum_fold_regr << "\n";
        if (print)
            cout << "call prob: " << call << " fold prob: " << fold << "\n";

        Utility curr_util = call * call_util + fold * fold_util;

        if (print)
            cout << "curr util: " << curr_util << "\n";

        // update regrets based on utils
        // note: as this is player two, who is trying to minimize utility
        // regrets are in terms of negative utility
        float call_regr = call_util < curr_util ? curr_util - call_util : 0.0;
        float fold_regr = fold_util < curr_util ? curr_util - fold_util : 0.0;

        first_bet_response[(card_2 - 1) * 2].sum_regret += prob * call_regr;
        first_bet_response[(card_2 - 1) * 2 + 1].sum_regret += prob * fold_regr;

        if (print)
            cout << "call regr: " << call_regr << " fold regr: " << fold_regr << "\n";

        if (print) cout << "\n";

        return curr_util;
    }

    Utility get_check_util(int card_1, int card_2) {
        return card_1 > card_2 ? 1.0 : -1.0;
    }

    // Train this outcome (and it's descendants) once and return utility
    Utility train_first_action(int card_1, int card_2, float prob, bool print = false) {
        // probs in our current strat
        float sum_bet_regr = first_action[(card_1 - 1) * 2].sum_regret;
        float sum_check_regr = first_action[(card_1 - 1) * 2 + 1].sum_regret;

        float sum_regret = sum_bet_regr + sum_check_regr;

        float bet = sum_bet_regr / sum_regret;
        float check = sum_check_regr / sum_regret;

        // recalc utility of each action
        Utility bet_util = train_bet_response(card_1, card_2, bet * prob);
        Utility check_util = get_check_util(card_1, card_2);

        if (print) {
            cout << "bet util: " << bet_util << " check util: " << check_util << "\n";
        }

        Utility curr_util = bet * bet_util + check * check_util;

        // update regrets based on utils
        float bet_regr = bet_util > curr_util ? bet_util - curr_util : 0.0;
        float check_regr = check_util > curr_util ? check_util - curr_util : 0.0;

        first_action[(card_1 - 1) * 2].sum_regret += prob * bet_regr;
        first_action[(card_1 - 1) * 2 + 1].sum_regret += prob * check_regr;

        // return util (calced on old values)
        return curr_util;
    }

    // Train the entire game once
    Utility train_game(bool print = false) {
        Utility running_util;

        for (int card_1 = 1; card_1 <= 3; card_1++) {
            for (int card_2 = 1; card_2 <= 3; card_2++) {
                if (card_1 == card_2) continue;

                Utility cur_util = train_first_action(card_1, card_2, get_init_prob(card_1, card_2));

                if (print)
                    cout << card_1 << " " << card_2 << ": " << cur_util << "\n";

                running_util += cur_util * get_init_prob(card_1, card_2);
            }
        }

        return running_util;
    }

    // Print the game tree
    void print() {
        float sum_reg1 = first_action[0].sum_regret + first_action[1].sum_regret;
        float sum_reg2 = first_action[2].sum_regret + first_action[3].sum_regret;
        float sum_reg3 = first_action[4].sum_regret + first_action[5].sum_regret;

        cout << "Player 1:\n";
        cout << "1:\n\tBet:" <<
            first_action[0].sum_regret / sum_reg1 << "\n\tCheck:" <<
            first_action[1].sum_regret / sum_reg1 << "\n";
        cout << "2:\n\tBet:" <<
            first_action[2].sum_regret / sum_reg2 << "\n\tCheck:" <<
            first_action[3].sum_regret / sum_reg2 << "\n";
        cout << "3:\n\tBet:" <<
            first_action[4].sum_regret / sum_reg3 << "\n\tCheck:" <<
            first_action[5].sum_regret / sum_reg3 << "\n";

        sum_reg1 = first_bet_response[0].sum_regret + first_bet_response[1].sum_regret;
        sum_reg2 = first_bet_response[2].sum_regret + first_bet_response[3].sum_regret;
        sum_reg3 = first_bet_response[4].sum_regret + first_bet_response[5].sum_regret;

        cout << "\n";
        cout << "Player 2's response to a bet:\n";
        cout << "1:\n\tCall:" <<
            first_bet_response[0].sum_regret / sum_reg1 << "\n\tFold:" <<
            first_bet_response[1].sum_regret / sum_reg1 << "\n";
        cout << "2:\n\tCall:" <<
            first_bet_response[2].sum_regret / sum_reg2 << "\n\tFold:" <<
            first_bet_response[3].sum_regret / sum_reg2 << "\n";
        cout << "3:\n\tCall:" <<
            first_bet_response[4].sum_regret / sum_reg3 << "\n\tFold:" <<
            first_bet_response[5].sum_regret / sum_reg3 << "\n";
    }
};

int main() {
    GameTree gt;

    cout << "initial strategies:\n";
    gt.print();

    for (int i = 0; i < 10'000'000; i++)
        gt.train_game();

    cout << "Ending utility: " << gt.train_game(false) << "\n\n";

    cout << "Ending utility per initial game state:\n";
    gt.train_game(true);
    cout << "\n";

    cout << "Ending optimal strategies:\n";
    gt.print();
}