This will eventually be a C++ implementation of Counterfactual Regret Minimization Algorithm
to solve Poker.

Currently, this is a demo in which we solve a simple game where player 1 can bet or check
and player 2 can either call or fold facing a bet.

This demos the information set and an unstructured game-tree.

Future work will be to standardize the execution of CFR and to template the game tree
on a node data structure. There is also need for a gui to view resultant strategies and
input intial distributions, as well as a way to tabulate basic stats from the game tree
data structure (mainly nash-distance).

Under current settings, it gets a value of ~0.05 of the game, from a strategy of 
never betting 2, betting 1 about 1/3, always bet 3. Then player 2 always calls with 3
and always folds with 1, and calls with 2 about 1/2 of the time.
My estimate of Maximally exploitative strategies seem like these are pretty close to Equilibrium