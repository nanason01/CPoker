
all:
	g++ -std=c++2a -c ContinuationNode.cc -g3
	g++ -std=c++2a runner.cc ContinuationNode.o -g3 -o runner

fast:
	g++ -std=c++2a -O3 -c ContinuationNode.cc
	g++ -std=c++2a -O3 runner.cc ContinuationNode.o  -o runner

debug:
	g++ -std=c++2a runner.cc -DDEBUG -g3 -o runner
