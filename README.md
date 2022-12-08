This will eventually be a C++ implementation of Counterfactual Regret Minimization Algorithm to solve Poker.

demo/ is the dir for the initial demo of the CFR algorithm in a very simple context.

dynamic_round/ is the dir for a dynamically generated game tree (and dynamic_round/old/ is the dir for the first iteration, incorrect dynamically generated game tree).

template_round/ is the dir for a statically generated game tree. I think a lot of it is very interesting functionality, but it's likely a dead-end as it was built without the top-down design in mind.

# Version history

## v0.0.1
Under current settings, it gets a value of ~0.05 of the game, from a strategy of 
never betting 2, betting 1 about 1/3, always bet 3. Then player 2 always calls with 3
and always folds with 1, and calls with 2 about 1/2 of the time.
My estimate of Maximally exploitative strategies seem like these are pretty close to Equilibrium

## v0.0.2
Under current settings, it plays the classic QKA game, where player 1 gets either a Q or an A,
and player 2 always gets a K. 1 can bet or check, and 2 can call or fold. We achieve the correct
outcome iteratively.

## v0.1.0
template_round/ demonstrates a lot of strategies for unpacking variadic templates as well as simplifying the representation of compile time computations. Although there isn't much useful functionality, it represents the game well and statically, so it will be a useful resource in the future.

## v0.8.0
We can now (inefficiently) represent and optimize game trees using a node data structure. We can also query the nash-distance of the output strategy to evaluate automatically how much loss the strategy has.

## v0.8.1
Previously, CFR was being incorrecly computed. This is a good excuse to rewrite a lot of the code in dynamic_round as it is quite messy and error-prone. The existing code was moved to old/, and the new code is what stands in dynamic_round. It now (correctly) computes optimal strategies for a dynamically generated game tree. It also can check the nash distance of it's output. 

# Issues
There is a small problem with certain game trees where a child of a node which is hardly reached will freeze and all of it's children will be exploitable. This is my understanding of the bug, I will have to look more into it later, but the strategies themselves *seem* reasonable.

# Future direction
My main goal currently is to statically generate a game tree. To more accurately represent poker, the simplified game will eventually have to include chance nodes representing a card draw, as well as computing hand rankings and accounting for ties, but this isn't part of the hot path nor main idea, so it's on the shelf for now. I think the hand idx abstraction is enough of a challenge for now.
Very far into the future, I would like to experiment with using GPUs to parallelize the traversal. With ~50^4 starting states in poker, I imagine there's a lot of room for this.
