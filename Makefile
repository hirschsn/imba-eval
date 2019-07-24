
all:
	g++ -O3 -g3 -ggdb --std=c++17 imba-eval.cc -o imba-eval -pthread -lboost_program_options