all:
	clang++ main.cc sha256.cpp -c
	clang++ main.o sha256.o -o main
